#include <script-nui-components/popup_menu.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/frontend/api/event.hpp>
#include <nui/frontend/api/abort_controller.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/frontend/utility/functions.hpp>
#include <nui/frontend/utility/body_portal.hpp>

#include <emscripten/val.h>
#include <fmt/format.h>

using namespace Nui::Elements;
using namespace Nui::Attributes;

// ── Internal geometry helpers ────────────────────────────────────────────────

namespace ScriptNuiComponents
{
    namespace
    {
        struct Rect
        {
            double left{};
            double top{};
            double right{};
            double bottom{};
            double width() const
            {
                return right - left;
            }
            double height() const
            {
                return bottom - top;
            }
        };

        struct Size
        {
            double width{};
            double height{};
        };

        /// Read the DOMRect of any element via getBoundingClientRect().
        Rect getBoundingRect(emscripten::val const& el)
        {
            const auto rect = el.call<emscripten::val>("getBoundingClientRect");
            return {
                rect["left"].as<double>(),
                rect["top"].as<double>(),
                rect["right"].as<double>(),
                rect["bottom"].as<double>(),
            };
        }

        /// Viewport size via window.innerWidth / innerHeight.
        Size viewportSize()
        {
            const auto win = emscripten::val::global("window");
            return {win["innerWidth"].as<double>(), win["innerHeight"].as<double>()};
        }

        /// Small gap between anchor edge and menu edge (px).
        constexpr double kGap = 4.0;
        /// Minimum margin from viewport edges (px).
        constexpr double kMargin = 6.0;

        /// Compute the best (left, top) position for a menu of `menu` size,
        /// anchored to `anchor`, inside a viewport of `vp` size.
        /// Priority: below-left, below-right, above-left, above-right, right, left,
        /// then clamped fallback.
        std::pair<double, double> computeOptimalPosition(Rect const& anchor, Size const& menu, Size const& vp)
        {
            // Candidate positions: {left, top}
            // Each is checked to see whether the menu fits without clipping.

            auto fits = [&](double x, double y) -> bool
            {
                return x >= kMargin && y >= kMargin && (x + menu.width) <= (vp.width - kMargin) &&
                    (y + menu.height) <= (vp.height - kMargin);
            };

            // 1. Below – left-aligned
            {
                const double x = anchor.left;
                const double y = anchor.bottom + kGap;
                if (fits(x, y))
                    return {x, y};
            }
            // 2. Below – right-aligned
            {
                const double x = anchor.right - menu.width;
                const double y = anchor.bottom + kGap;
                if (fits(x, y))
                    return {x, y};
            }
            // 3. Above – left-aligned
            {
                const double x = anchor.left;
                const double y = anchor.top - kGap - menu.height;
                if (fits(x, y))
                    return {x, y};
            }
            // 4. Above – right-aligned
            {
                const double x = anchor.right - menu.width;
                const double y = anchor.top - kGap - menu.height;
                if (fits(x, y))
                    return {x, y};
            }
            // 5. Right of anchor, top-aligned
            {
                const double x = anchor.right + kGap;
                const double y = anchor.top;
                if (fits(x, y))
                    return {x, y};
            }
            // 6. Left of anchor, top-aligned
            {
                const double x = anchor.left - kGap - menu.width;
                const double y = anchor.top;
                if (fits(x, y))
                    return {x, y};
            }
            // 7. Fallback: clamp into viewport with margin
            {
                const double x = std::max(kMargin, std::min(anchor.left, vp.width - kMargin - menu.width));
                const double y = std::max(kMargin, std::min(anchor.bottom + kGap, vp.height - kMargin - menu.height));
                return {x, y};
            }
        }

    } // namespace

    // ── Implementation ───────────────────────────────────────────────────────────

    struct PopupMenu::Implementation
    {
        std::vector<Entry> items;
        Nui::Observed<bool> visible{false};
        Nui::Observed<double> posX{0.0};
        Nui::Observed<double> posY{0.0};
        Nui::Observed<bool> measuring{false};

        // Stable-lifetime index vector for Nui::range inside the render lambda.
        // Must outlive every render pass — keeping it on the heap here guarantees
        // that.  Rebuilt whenever items changes.
        Nui::Observed<std::vector<std::size_t>> indices;

        // Body portal: the menu is rendered as a direct child of document.body so
        // that position:fixed is relative to the viewport, not to any ancestor
        // with a CSS transform/filter/perspective.
        std::optional<Nui::BodyPortal> portal;

        // Used to remove the document-level outside-click listener when the menu
        // closes.  A full-viewport backdrop div is intentionally NOT used: the
        // backdrop would sit above other interactive elements (e.g. dropdown
        // trigger buttons) in the portal stacking context, forcing two clicks to
        // switch menus.  Instead we rely on a document-level click listener for
        // "click outside" detection.
        Nui::WebApi::AbortController outsideClickAbort{};

        void attachOutsideClickListener()
        {
            // Abort any previous listener before creating a new controller.
            outsideClickAbort.abort();
            outsideClickAbort = Nui::WebApi::AbortController{};
            auto options = emscripten::val::object();
            options.set("signal", outsideClickAbort.signal().val());
            auto* selfImpl = this;
            emscripten::val::global("document")
                .call<void>(
                    "addEventListener",
                    std::string{"click"},
                    Nui::bind(
                        [selfImpl](emscripten::val /*event*/)
                        {
                            selfImpl->visible = false;
                            selfImpl->measuring = false;
                            selfImpl->outsideClickAbort.abort();
                            Nui::globalEventContext.executeActiveEventsImmediately();
                        },
                        std::placeholders::_1
                    ),
                    options
                );
        }

        void detachOutsideClickListener()
        {
            outsideClickAbort.abort();
        }

        void rebuildIndices()
        {
            indices.value().resize(items.size());
            for (std::size_t i = 0; i < items.size(); ++i)
                indices.value()[i] = i;
            indices.modify();
        }

        // Renders a single MenuItem row
        Nui::ElementRenderer renderItem(MenuItem const& mi)
        {
            using Nui::Elements::div;
            using Nui::Elements::span;

            std::string itemClass = "script-nui-popup-menu__item";
            if (mi.disabled)
                itemClass += " script-nui-popup-menu__item--disabled";

            auto iconCell = [&]() -> Nui::ElementRenderer
            {
                if (mi.icon.empty())
                    return span{class_ = "script-nui-popup-menu__icon script-nui-popup-menu__icon--empty"}();
                return span{class_ = "script-nui-popup-menu__icon"}(mi.icon);
            }();

            auto labelCell = [&]() -> Nui::ElementRenderer
            {
                if (mi.shortcut.empty())
                    return span{class_ = "script-nui-popup-menu__label"}(mi.label);
                return span{
                    class_ = "script-nui-popup-menu__label"
                }(text{mi.label}(), span{class_ = "script-nui-popup-menu__shortcut"}(mi.shortcut));
            }();

            if (mi.disabled)
                return div{class_ = itemClass}(std::move(iconCell), std::move(labelCell));

            auto action = mi.action;
            return div{
                class_ = itemClass,
                onClick = [action, impl = this](Nui::val event)
                {
                    event.call<void>("stopPropagation");
                    impl->visible = false;
                    impl->measuring = false;
                    impl->detachOutsideClickListener();
                    Nui::globalEventContext.executeActiveEventsImmediately();
                    if (action)
                        action();
                }
            }(std::move(iconCell), std::move(labelCell));
        }

        Nui::ElementRenderer renderEntry(Entry const& entry)
        {
            using Nui::Elements::div;
            using Nui::Elements::span;

            return std::visit(
                [this](auto const& e) -> Nui::ElementRenderer
                {
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, MenuItem>)
                        return renderItem(e);
                    else if constexpr (std::is_same_v<T, Separator>)
                        return div{class_ = "script-nui-popup-menu__separator"}();
                    else
                        return div{class_ = "script-nui-popup-menu__section-header"}(text{e.title}());
                },
                entry
            );
        }
    };

    // ── PopupMenu public API ─────────────────────────────────────────────────────

    PopupMenu::PopupMenu()
        : impl_{std::make_unique<Implementation>()}
    {}
    PopupMenu::~PopupMenu() = default;
    PopupMenu::PopupMenu(PopupMenu&&) = default;
    PopupMenu& PopupMenu::operator=(PopupMenu&&) = default;

    void PopupMenu::setItems(std::vector<Entry> items)
    {
        impl_->items = std::move(items);
        impl_->rebuildIndices();
    }

    void PopupMenu::open(double x, double y)
    {
        impl_->measuring = false;
        impl_->posX = x;
        impl_->posY = y;
        impl_->visible = true;
        impl_->attachOutsideClickListener();
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    void PopupMenu::openNextTo(std::string anchorId)
    {
        auto* impl = impl_.get();

        // ── Phase 1: measure pass ────────────────────────────────────────────
        // Render the menu off-screen so the browser assigns it real layout
        // dimensions.  The CSS class "script-nui-popup-menu--measuring" enforces
        //   visibility:hidden; left:-9999px; top:-9999px; animation:none
        // so the element participates in layout without being seen.
        impl->posX = -9999.0;
        impl->posY = -9999.0;
        impl->measuring = true;
        impl->visible = true;
        Nui::globalEventContext.executeActiveEventsImmediately();

        // ── Phase 2: position after one rAF tick ────────────────────────────
        // Only the anchorId string and impl* are captured — no emscripten::val
        // in the closure, which avoids handle-table lifetime issues across the
        // async boundary entirely.  The element is looked up fresh inside the
        // rAF callback; it is guaranteed to still be in the DOM at that point
        // (it is the element the user just interacted with).
        emscripten::val::global("window").call<void>(
            "requestAnimationFrame",
            Nui::bind(
                [impl, anchorId](Nui::val /*timestamp*/) mutable
                {
                    const auto doc = emscripten::val::global("document");
                    const auto anchorEl = doc.call<emscripten::val>("getElementById", emscripten::val(anchorId));
                    const auto menuEl = doc.call<emscripten::val>(
                        "querySelector", emscripten::val(".script-nui-popup-menu--measuring")
                    );

                    if (anchorEl.isNull() || anchorEl.isUndefined())
                    {
                        // The id wasn't found — caller passed a wrong id.
                        // Close the measuring pass so we don't leave a hidden
                        // ghost menu in the DOM.
                        Nui::WebApi::Console::error("PopupMenu::openNextTo failed: no element with id '{}'", anchorId);
                        impl->visible = false;
                        impl->measuring = false;
                        Nui::globalEventContext.executeActiveEventsImmediately();
                        return;
                    }

                    const Rect anchor = getBoundingRect(anchorEl);

                    if (!menuEl.isNull() && !menuEl.isUndefined())
                    {
                        const Rect menuR = getBoundingRect(menuEl);
                        const Size menu = {menuR.width(), menuR.height()};
                        const Size vp = viewportSize();
                        const auto [x, y] = computeOptimalPosition(anchor, menu, vp);
                        impl->posX = x;
                        impl->posY = y;
                    }
                    else
                    {
                        // Fallback: best-effort below-left, no clamping.
                        impl->posX = anchor.left;
                        impl->posY = anchor.bottom + kGap;
                    }

                    impl->measuring = false;
                    // Attach the outside-click listener here, after the rAF tick,
                    // so the click that opened the menu has already finished bubbling
                    // and does not immediately close it.
                    impl->attachOutsideClickListener();
                    Nui::WebApi::Console::log(
                        fmt::format(
                            "PopupMenu::openNextTo anchorId='{}' anchorRect={{left:{:.1f}, top:{:.1f}, right:{:.1f}, "
                            "bottom:{:.1f}}} finalPos={{x:{:.1f}, y:{:.1f}}}",
                            anchorId,
                            anchor.left,
                            anchor.top,
                            anchor.right,
                            anchor.bottom,
                            impl->posX.value(),
                            impl->posY.value()
                        )
                    );
                    Nui::globalEventContext.executeActiveEventsImmediately();
                },
                std::placeholders::_1
            )
        );
    }

    void PopupMenu::openAt(double x, double y)
    {
        auto* impl = impl_.get();

        // Phase 1: off-screen measuring pass
        impl->posX = -9999.0;
        impl->posY = -9999.0;
        impl->measuring = true;
        impl->visible = true;
        Nui::globalEventContext.executeActiveEventsImmediately();

        // Phase 2: clamp into viewport after one rAF tick
        emscripten::val::global("window").call<void>(
            "requestAnimationFrame",
            Nui::bind(
                [impl, x, y](Nui::val /*timestamp*/) mutable
                {
                    const auto doc = emscripten::val::global("document");
                    const auto menuEl = doc.call<emscripten::val>(
                        "querySelector", emscripten::val(".script-nui-popup-menu--measuring")
                    );

                    const Size vp = viewportSize();

                    if (!menuEl.isNull() && !menuEl.isUndefined())
                    {
                        const Rect menuR = getBoundingRect(menuEl);
                        const Size menu = {menuR.width(), menuR.height()};
                        // Treat the cursor as a zero-size anchor point so the menu
                        // opens below/right of the cursor by default and flips when
                        // it would overflow the viewport.
                        const Rect anchor{x, y, x, y};
                        const auto [fx, fy] = computeOptimalPosition(anchor, menu, vp);
                        impl->posX = fx;
                        impl->posY = fy;
                    }
                    else
                    {
                        // Fallback: simple clamp with margin
                        impl->posX = std::max(kMargin, std::min(x, vp.width - kMargin));
                        impl->posY = std::max(kMargin, std::min(y, vp.height - kMargin));
                    }

                    impl->measuring = false;
                    // Attach after the rAF tick so the triggering click has
                    // already finished bubbling and won't immediately close the menu.
                    impl->attachOutsideClickListener();
                    Nui::globalEventContext.executeActiveEventsImmediately();
                },
                std::placeholders::_1
            )
        );
    }

    void PopupMenu::close()
    {
        impl_->detachOutsideClickListener();
        impl_->visible = false;
        impl_->measuring = false;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    bool PopupMenu::isOpen() const
    {
        return impl_->visible.value();
    }

    void PopupMenu::modifyItemByLabel(std::string const& label, std::function<void(MenuItem*)> modifier)
    {
        for (std::size_t i = 0; i < impl_->items.size(); ++i)
        {
            if (auto* mi = std::get_if<MenuItem>(&impl_->items[i]); mi != nullptr && mi->label == label)
            {
                modifier(mi);
                if (i <= impl_->indices.size())
                {
                    impl_->indices[i] = i;
                    impl_->indices.eventContext().sync();
                }
                else
                    impl_->rebuildIndices();
                return;
            }
        }
    }

    // ── Render ───────────────────────────────────────────────────────────────────

    Nui::ElementRenderer PopupMenu::operator()(std::vector<Nui::Attribute> additionalAttributes)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;

        auto* impl = impl_.get();

        if (!impl->portal)
        {
            additionalAttributes.push_back(class_ = "script-nui-popup-menu-container");

            // Observe both `visible` and `measuring` so the render lambda re-runs
            // when the rAF callback flips measuring=false and sets the final position.
            // Observing only `visible` would mean the lambda fires once (Phase 1,
            // position=-9999, measuring=true) and never again — the menu stays hidden.
            impl->portal.emplace(
                div{
                    std::move(additionalAttributes)
                }(Nui::observe(impl->visible, impl->measuring),
                    [impl](bool const& isVisible, bool const& isMeasuring) -> Nui::ElementRenderer
                    {
                        if (!isVisible)
                            return div{style = "display:none;"}();

                        const std::string posStyle =
                            fmt::format("left:{:.1f}px;top:{:.1f}px;", impl->posX.value(), impl->posY.value());

                        const std::string menuClass = isMeasuring
                            ? "script-nui-popup-menu script-nui-popup-menu--measuring"
                            : "script-nui-popup-menu";

                        return div{
                            class_ = menuClass,
                            style = posStyle,
                            "click"_event =
                                [](Nui::WebApi::Event event)
                            {
                                // Prevent clicks inside the menu from bubbling to
                                // the document-level outside-click listener.
                                event.stopPropagation();
                            }
                        }(Nui::range(impl->indices),
                            [impl](long long /*i*/, std::size_t const& idx) -> Nui::ElementRenderer
                            {
                                return impl->renderEntry(impl->items[idx]);
                            });
                    })
            );
        }

        // The actual menu DOM lives in document.body via the portal.
        // Return nil() as a placeholder in the Nui component tree.
        return Nui::nil();
    }
}