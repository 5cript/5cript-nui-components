#include <script-nui-components/popup_menu.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/frontend/api/event.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/frontend/utility/functions.hpp>

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
                return span{class_ = "script-nui-popup-menu__icon"}(text{mi.icon}());
            }();

            auto labelCell = [&]() -> Nui::ElementRenderer
            {
                if (mi.shortcut.empty())
                    return span{class_ = "script-nui-popup-menu__label"}(text{mi.label}());
                return span{
                    class_ = "script-nui-popup-menu__label"
                }(text{mi.label}(), span{class_ = "script-nui-popup-menu__shortcut"}(mi.shortcut));
            }();

            if (mi.disabled)
                return div{class_ = itemClass}(std::move(iconCell), std::move(labelCell));

            auto action = mi.action;
            return div{
                class_ = itemClass,
                onClick = [action](Nui::val /*event*/)
                {
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
                    Nui::globalEventContext.executeActiveEventsImmediately();
                },
                std::placeholders::_1
            )
        );
    }

    void PopupMenu::close()
    {
        impl_->visible = false;
        impl_->measuring = false;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    bool PopupMenu::isOpen() const
    {
        return impl_->visible.value();
    }

    // ── Render ───────────────────────────────────────────────────────────────────

    Nui::ElementRenderer PopupMenu::operator()()
    {
        using Nui::Elements::div;
        using Nui::Elements::span;

        auto* impl = impl_.get();

        // Observe both `visible` and `measuring` so the render lambda re-runs
        // when the rAF callback flips measuring=false and sets the final position.
        // Observing only `visible` would mean the lambda fires once (Phase 1,
        // position=-9999, measuring=true) and never again — the menu stays hidden.
        return div{}(
            Nui::observe(impl->visible, impl->measuring),
            [impl](bool const& isVisible, bool const& isMeasuring) -> Nui::ElementRenderer
            {
                if (!isVisible)
                    return div{style = "display:none;"}();

                const std::string posStyle =
                    fmt::format("left:{:.1f}px;top:{:.1f}px;", impl->posX.value(), impl->posY.value());

                const std::string menuClass =
                    isMeasuring ? "script-nui-popup-menu script-nui-popup-menu--measuring" : "script-nui-popup-menu";

                auto backdrop =
                    div{style = "position:fixed;inset:0;z-index:9998;",
                        onClick = [impl](Nui::val /*event*/)
                        {
                            impl->visible = false;
                            impl->measuring = false;
                        }}();

                return div{
                    style = "contents;"
                }(std::move(backdrop),
                    div{
                        class_ = menuClass,
                        style = posStyle,
                        "click"_event =
                            [](Nui::WebApi::Event event)
                        {
                            event.stopPropagation();
                        }
                    }(Nui::range(impl->indices),
                        [impl](long long /*i*/, std::size_t const& idx) -> Nui::ElementRenderer
                        {
                            return impl->renderEntry(impl->items[idx]);
                        }));
            }
        );
    }
}