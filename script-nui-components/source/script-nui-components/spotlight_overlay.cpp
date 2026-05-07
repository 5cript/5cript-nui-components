#include <script-nui-components/spotlight_overlay.hpp>
#include <script-nui-components/utf8.hpp>

#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/api/abort_controller.hpp>
#include <nui/frontend/api/resize_observer.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/elements/svg/defs.hpp>
#include <nui/frontend/elements/svg/g.hpp>
#include <nui/frontend/elements/svg/mask.hpp>
#include <nui/frontend/elements/svg/path.hpp>
#include <nui/frontend/elements/svg/rect.hpp>
#include <nui/frontend/elements/svg/svg.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/attributes/svg/d.hpp>
#include <nui/frontend/attributes/svg/fill.hpp>
#include <nui/frontend/attributes/svg/fill_opacity.hpp>
#include <nui/frontend/attributes/svg/height.hpp>
#include <nui/frontend/attributes/svg/mask.hpp>
#include <nui/frontend/attributes/svg/rx.hpp>
#include <nui/frontend/attributes/svg/ry.hpp>
#include <nui/frontend/attributes/svg/transform.hpp>
#include <nui/frontend/attributes/svg/width.hpp>
#include <nui/frontend/attributes/svg/x.hpp>
#include <nui/frontend/attributes/svg/y.hpp>
#include <nui/frontend/utility/body_portal.hpp>
#include <nui/frontend/val.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace ScriptNuiComponents
{
    namespace
    {
        constexpr double viewportMargin = 12.0;
        constexpr double tooltipEstimatedWidth = 300.0;
        constexpr double tooltipEstimatedHeight = 170.0;

        /** @brief Side enum → CSS-friendly string for the data-side attribute. */
        const char* sideToString(SpotlightSide side)
        {
            switch (side)
            {
                case SpotlightSide::Top:
                    return "top";
                case SpotlightSide::Right:
                    return "right";
                case SpotlightSide::Bottom:
                    return "bottom";
                case SpotlightSide::Left:
                    return "left";
                case SpotlightSide::Auto:
                    return "auto";
            }
            return "auto";
        }

        unsigned& instanceCounter()
        {
            static unsigned counter = 0;
            return counter;
        }
    } // namespace

    struct SpotlightOverlay::Implementation
    {
        struct FreeSpace
        {
            double top{0.0};
            double bottom{0.0};
            double left{0.0};
            double right{0.0};
        };

        struct Geometry
        {
            double targetX{0.0};
            double targetY{0.0};
            double targetW{0.0};
            double targetH{0.0};
            double tooltipX{0.0};
            double tooltipY{0.0};
            double tooltipW{tooltipEstimatedWidth};
            double tooltipH{tooltipEstimatedHeight};
            SpotlightSide chosenSide{SpotlightSide::Bottom};
            bool valid{false};
        };

        SpotlightOptions currentOptions{};
        Nui::Observed<bool> visible{false};
        Nui::Observed<Geometry> geometry{};

        Nui::WebApi::AbortController listenerAbort{};
        std::optional<Nui::WebApi::ResizeObserver> resizeObserver{};
        Nui::val mutationObserver{Nui::val::undefined()};
        std::optional<Nui::BodyPortal> portal{};
        bool rafScheduled{false};
        unsigned instanceId{0};

        std::string maskId() const
        {
            return fmt::format("snc-spotlight-hole-{}", instanceId);
        }

        Nui::val findTarget() const
        {
            if (currentOptions.targetElementId.empty())
                return Nui::val::null();
            return Nui::val::global("document").call<Nui::val>("getElementById", currentOptions.targetElementId);
        }

        // ── Geometry math ─────────────────────────────────────────────

        FreeSpace computeFreeSpace(double tx, double ty, double tw, double th, double vw, double vh) const
        {
            return FreeSpace{
                .top = ty - viewportMargin,
                .bottom = vh - (ty + th) - viewportMargin,
                .left = tx - viewportMargin,
                .right = vw - (tx + tw) - viewportMargin,
            };
        }

        SpotlightSide chooseSide(FreeSpace const& free, double tipW, double tipH) const
        {
            const double reqV = tipH + currentOptions.tooltipGap;
            const double reqH = tipW + currentOptions.tooltipGap;

            const auto score = [&](SpotlightSide side) -> double
            {
                switch (side)
                {
                    case SpotlightSide::Top:
                        return free.top - reqV;
                    case SpotlightSide::Bottom:
                        return free.bottom - reqV;
                    case SpotlightSide::Left:
                        return free.left - reqH;
                    case SpotlightSide::Right:
                        return free.right - reqH;
                    case SpotlightSide::Auto:
                        return 0.0;
                }
                return 0.0;
            };

            if (currentOptions.preferredSide != SpotlightSide::Auto && score(currentOptions.preferredSide) >= 0.0)
                return currentOptions.preferredSide;

            constexpr SpotlightSide all[] = {
                SpotlightSide::Bottom,
                SpotlightSide::Top,
                SpotlightSide::Right,
                SpotlightSide::Left,
            };
            SpotlightSide best = all[0];
            double bestScore = score(all[0]);
            for (auto side : all)
            {
                const double s = score(side);
                if (s > bestScore)
                {
                    bestScore = s;
                    best = side;
                }
            }
            return best;
        }

        std::pair<double, double> placeTooltipOnSide(
            SpotlightSide side,
            double tx,
            double ty,
            double tw,
            double th,
            double tipW,
            double tipH
        ) const
        {
            const double gap = currentOptions.tooltipGap;
            switch (side)
            {
                case SpotlightSide::Top:
                    return {tx + tw / 2.0 - tipW / 2.0, ty - gap - tipH};
                case SpotlightSide::Bottom:
                    return {tx + tw / 2.0 - tipW / 2.0, ty + th + gap};
                case SpotlightSide::Left:
                    return {tx - gap - tipW, ty + th / 2.0 - tipH / 2.0};
                case SpotlightSide::Right:
                    return {tx + tw + gap, ty + th / 2.0 - tipH / 2.0};
                case SpotlightSide::Auto:
                    return {tx, ty};
            }
            return {tx, ty};
        }

        std::pair<double, double>
        clampTooltipToViewport(double x, double y, double tipW, double tipH, double vw, double vh) const
        {
            return {
                std::clamp(x, viewportMargin, vw - tipW - viewportMargin),
                std::clamp(y, viewportMargin, vh - tipH - viewportMargin),
            };
        }

        // ── Lifecycle ─────────────────────────────────────────────────

        void scheduleReposition()
        {
            if (rafScheduled)
                return;
            rafScheduled = true;
            Nui::val::global("requestAnimationFrame")(Nui::bind(
                [this](Nui::val)
                {
                    rafScheduled = false;
                    if (visible.value())
                        recompute();
                },
                std::placeholders::_1
            ));
        }

        /** @brief Whether two geometries differ enough that a re-render is
         *         worth doing. Sub-pixel jitter is suppressed so the
         *         MutationObserver-driven feedback loop (every render
         *         mutates the body subtree, which retriggers
         *         scheduleReposition) terminates after one stable pass —
         *         otherwise click handlers churn at 60 Hz and never get
         *         the chance to fire. */
        static bool geometryChanged(Geometry const& a, Geometry const& b)
        {
            constexpr double epsilon = 0.5;
            if (a.valid != b.valid)
                return true;
            if (a.chosenSide != b.chosenSide)
                return true;
            return std::abs(a.targetX - b.targetX) >= epsilon || std::abs(a.targetY - b.targetY) >= epsilon ||
                std::abs(a.targetW - b.targetW) >= epsilon || std::abs(a.targetH - b.targetH) >= epsilon ||
                std::abs(a.tooltipX - b.tooltipX) >= epsilon || std::abs(a.tooltipY - b.tooltipY) >= epsilon ||
                std::abs(a.tooltipW - b.tooltipW) >= epsilon || std::abs(a.tooltipH - b.tooltipH) >= epsilon;
        }

        void recompute()
        {
            if (!visible.value())
                return;

            const auto previous = geometry.value();
            const auto target = findTarget();
            if (target.isNull() || target.isUndefined())
            {
                if (!previous.valid)
                    return;
                Geometry invalidated = previous;
                invalidated.valid = false;
                geometry = invalidated;
                geometry.modifyNow();
                return;
            }

            const auto window = Nui::val::global("window");
            const auto rect = target.call<Nui::val>("getBoundingClientRect");
            const double tx = rect["left"].as<double>();
            const double ty = rect["top"].as<double>();
            const double tw = rect["width"].as<double>();
            const double th = rect["height"].as<double>();
            const double vw = window["innerWidth"].as<double>();
            const double vh = window["innerHeight"].as<double>();

            const double tipW = previous.tooltipW > 0.0 ? previous.tooltipW : tooltipEstimatedWidth;
            const double tipH = previous.tooltipH > 0.0 ? previous.tooltipH : tooltipEstimatedHeight;

            const auto free = computeFreeSpace(tx, ty, tw, th, vw, vh);
            const auto chosen = chooseSide(free, tipW, tipH);
            auto [tipX, tipY] = placeTooltipOnSide(chosen, tx, ty, tw, th, tipW, tipH);
            std::tie(tipX, tipY) = clampTooltipToViewport(tipX, tipY, tipW, tipH, vw, vh);

            Geometry next{
                .targetX = tx,
                .targetY = ty,
                .targetW = tw,
                .targetH = th,
                .tooltipX = tipX,
                .tooltipY = tipY,
                .tooltipW = tipW,
                .tooltipH = tipH,
                .chosenSide = chosen,
                .valid = true,
            };
            const bool changed = geometryChanged(previous, next);
            if (changed)
            {
                geometry = next;
                geometry.modifyNow();
            }

            // Re-measuring the tooltip is only useful right after the
            // first valid layout (when the estimated dimensions get
            // replaced with the real ones). If geometry didn't change,
            // we've already converged — skip to break the loop.
            if (changed)
                measureTooltipNextFrame();
        }

        void measureTooltipNextFrame()
        {
            Nui::val::global("requestAnimationFrame")(Nui::bind(
                [this](Nui::val)
                {
                    if (!visible.value())
                        return;
                    const auto tooltipEl = Nui::val::global("document")
                                               .call<Nui::val>("querySelector", std::string{".snc-spotlight-tooltip"});
                    if (tooltipEl.isNull() || tooltipEl.isUndefined())
                        return;
                    const auto rect = tooltipEl.call<Nui::val>("getBoundingClientRect");
                    const double w = rect["width"].as<double>();
                    const double h = rect["height"].as<double>();
                    auto current = geometry.value();
                    if (std::abs(current.tooltipW - w) < 0.5 && std::abs(current.tooltipH - h) < 0.5)
                        return;
                    current.tooltipW = w;
                    current.tooltipH = h;
                    geometry = current;
                    geometry.modifyNow();
                    scheduleReposition();
                },
                std::placeholders::_1
            ));
        }

        // ── Listeners ─────────────────────────────────────────────────

        void installListeners()
        {
            const auto window = Nui::val::global("window");
            auto signalOptions = Nui::val::object();
            signalOptions.set("signal", listenerAbort.signal().val());

            window.call<void>(
                "addEventListener",
                std::string{"resize"},
                Nui::bind(
                    [this](Nui::val)
                    {
                        scheduleReposition();
                    },
                    std::placeholders::_1
                ),
                signalOptions
            );

            if (currentOptions.dismissOnEsc)
            {
                window.call<void>(
                    "addEventListener",
                    std::string{"keydown"},
                    Nui::bind(
                        [this](Nui::val event)
                        {
                            handleKeydown(std::move(event));
                        },
                        std::placeholders::_1
                    ),
                    signalOptions
                );
            }

            // ResizeObserver on the target — fires when the target itself
            // changes size (theme reflow, toolbar wrap, etc.).
            resizeObserver.emplace(
                [this](std::vector<Nui::WebApi::ResizeObserverEntry> const&, Nui::WebApi::ResizeObserver const&)
                {
                    scheduleReposition();
                }
            );
            const auto target = findTarget();
            if (!target.isNull() && !target.isUndefined())
                resizeObserver->observe(target);

            // MutationObserver on body — picks up dialog mounts (e.g.
            // settings opening between step 1 and step 2). Re-resolves
            // getElementById from scratch each pass.
            const auto mutationCtor = Nui::val::global("MutationObserver");
            if (!mutationCtor.isUndefined())
            {
                mutationObserver = mutationCtor.new_(
                    Nui::bind(
                        [this](Nui::val, Nui::val)
                        {
                            scheduleReposition();
                        },
                        std::placeholders::_1,
                        std::placeholders::_2
                    )
                );
                auto opts = Nui::val::object();
                opts.set("childList", true);
                opts.set("subtree", true);
                mutationObserver.call<void>("observe", Nui::val::global("document")["body"], opts);
            }
        }

        void teardownListeners()
        {
            // AbortController collapses every addEventListener call we
            // routed through `signal` in one shot — no per-listener
            // bookkeeping needed.
            listenerAbort.abort();
            listenerAbort = Nui::WebApi::AbortController{};

            if (resizeObserver)
            {
                resizeObserver->disconnect();
                resizeObserver.reset();
            }
            if (!mutationObserver.isUndefined() && !mutationObserver.isNull())
            {
                mutationObserver.call<void>("disconnect");
                mutationObserver = Nui::val::undefined();
            }
        }

        void handleKeydown(Nui::val event)
        {
            if (!visible.value())
                return;
            const auto key = event["key"].as<std::string>();
            if (key == "Escape" && currentOptions.onDismiss)
                currentOptions.onDismiss();
        }

        // ── Renderers (split for readability) ─────────────────────────

        Nui::ElementRenderer renderRoot();
        Nui::ElementRenderer renderBackdrop();
        Nui::ElementRenderer renderRing();
        Nui::ElementRenderer renderArrow();
        Nui::ElementRenderer renderTooltip();
    };

    // ── Render helpers ────────────────────────────────────────────────

    Nui::ElementRenderer SpotlightOverlay::Implementation::renderBackdrop()
    {
        namespace Svg = Nui::Elements::Svg;
        namespace SvgA = Nui::Attributes::Svg;
        using Nui::Attributes::class_;
        using Nui::Attributes::onClick;

        if (!visible.value())
            return Nui::nil();

        const auto& g = geometry.value();
        const double padding = currentOptions.cutoutPadding;
        const double holeX = g.targetX - padding;
        const double holeY = g.targetY - padding;
        const double holeW = std::max(0.0, g.targetW + padding * 2.0);
        const double holeH = std::max(0.0, g.targetH + padding * 2.0);
        const auto id = maskId();

        const auto onBackdropClick = [this]()
        {
            if (currentOptions.dismissOnBackdropClick && currentOptions.onDismiss)
                currentOptions.onDismiss();
        };

        // Click handler is on the inner dim rect, not the SVG container.
        // SVG <rect> uses `pointer-events: visiblePainted` by default, which
        // respects mask alpha — masked-out (cutout) pixels never intercept,
        // so clicks pass cleanly through to the highlighted target. The
        // SVG container itself is `pointer-events: none` (set in CSS) to
        // guarantee no WebKit edge-case lets it swallow events.
        return Svg::svg{
            class_ = "snc-spotlight-dim"
        }(Svg::defs{}(Svg::mask{Nui::Attributes::id = id}(
              Svg::rect{
                  SvgA::fill = std::string{"white"},
                  SvgA::x = std::string{"0"},
                  SvgA::y = std::string{"0"},
                  SvgA::width = std::string{"100%"},
                  SvgA::height = std::string{"100%"},
              }(),
              Svg::rect{
                  SvgA::fill = std::string{"black"},
                  SvgA::x = fmt::format("{}", holeX),
                  SvgA::y = fmt::format("{}", holeY),
                  SvgA::width = fmt::format("{}", holeW),
                  SvgA::height = fmt::format("{}", holeH),
                  SvgA::rx = std::string{"8"},
                  SvgA::ry = std::string{"8"},
              }()
          )),
            Svg::rect{
                class_ = "snc-spotlight-dim-fill",
                SvgA::fill = std::string{"black"},
                SvgA::fillOpacity = fmt::format("{:.3f}", currentOptions.dimOpacity),
                SvgA::x = std::string{"0"},
                SvgA::y = std::string{"0"},
                SvgA::width = std::string{"100%"},
                SvgA::height = std::string{"100%"},
                SvgA::mask = fmt::format("url(#{})", id),
                onClick = onBackdropClick,
            }());
    }

    Nui::ElementRenderer SpotlightOverlay::Implementation::renderRing()
    {
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        if (!visible.value())
            return Nui::nil();
        const auto& g = geometry.value();
        if (!g.valid)
            return Nui::nil();

        const double padding = currentOptions.cutoutPadding;
        return div{
            class_ = "snc-spotlight-ring",
            style = fmt::format(
                "left:{}px;top:{}px;width:{}px;height:{}px",
                g.targetX - padding,
                g.targetY - padding,
                std::max(0.0, g.targetW + padding * 2.0),
                std::max(0.0, g.targetH + padding * 2.0)
            ),
        }();
    }

    Nui::ElementRenderer SpotlightOverlay::Implementation::renderArrow()
    {
        namespace Svg = Nui::Elements::Svg;
        namespace SvgA = Nui::Attributes::Svg;
        using Nui::Attributes::class_;

        if (!visible.value())
            return Nui::nil();
        const auto& g = geometry.value();
        if (!g.valid)
            return Nui::nil();

        const double padding = currentOptions.cutoutPadding;
        double cx = 0.0;
        double cy = 0.0;
        int angleDeg = 0;
        switch (g.chosenSide)
        {
            case SpotlightSide::Left:
                cx = g.targetX - padding - 4.0;
                cy = g.targetY + g.targetH / 2.0;
                angleDeg = 0;
                break;
            case SpotlightSide::Right:
                cx = g.targetX + g.targetW + padding + 4.0;
                cy = g.targetY + g.targetH / 2.0;
                angleDeg = 180;
                break;
            case SpotlightSide::Top:
                cx = g.targetX + g.targetW / 2.0;
                cy = g.targetY - padding - 4.0;
                angleDeg = 90;
                break;
            case SpotlightSide::Bottom:
                cx = g.targetX + g.targetW / 2.0;
                cy = g.targetY + g.targetH + padding + 4.0;
                angleDeg = -90;
                break;
            case SpotlightSide::Auto:
                break;
        }

        return Svg::svg{class_ = "snc-spotlight-arrow"}(Svg::g{
            class_ = "snc-spotlight-arrow-pointer",
            SvgA::transform = fmt::format("translate({} {}) rotate({})", cx, cy, angleDeg),
        }(Svg::path{SvgA::d = std::string{"M 0 0 L -22 -14 L -16 0 L -22 14 Z"}}()));
    }

    Nui::ElementRenderer SpotlightOverlay::Implementation::renderTooltip()
    {
        using Nui::Elements::button;
        using Nui::Elements::div;
        using Nui::Elements::h3;
        using Nui::Elements::p;
        using Nui::Elements::span;
        using namespace Nui::Attributes;

        if (!visible.value())
            return Nui::nil();
        const auto& g = geometry.value();
        if (!g.valid)
            return Nui::nil();

        const auto& opts = currentOptions;
        const auto stepText = opts.stepCounter
            ? fmt::format("STEP {} OF {}", opts.stepCounter->current, opts.stepCounter->total)
            : std::string{};
        const auto onSkip = [this]()
        {
            if (currentOptions.onDismiss)
                currentOptions.onDismiss();
        };
        const auto onCta = [this]()
        {
            if (currentOptions.onAdvance)
                currentOptions.onAdvance();
        };

        // Wrap each optional element in a typed lambda so both arms of the
        // conditional collapse to a common ElementRenderer. Plain ternaries
        // hit "incompatible operand types" because each renderer is its own
        // unique lambda type.
        const auto titleEl = [&]() -> Nui::ElementRenderer
        {
            if (opts.title.empty())
                return Nui::nil();
            return h3{class_ = "snc-spotlight-title"}(opts.title);
        };
        const auto bodyEl = [&]() -> Nui::ElementRenderer
        {
            if (opts.bodyText.empty())
                return Nui::nil();
            return p{class_ = "snc-spotlight-body-text"}(opts.bodyText);
        };
        const auto skipEl = [&]() -> Nui::ElementRenderer
        {
            if (opts.skipLabel.empty())
                return Nui::nil();
            return button{class_ = "snc-spotlight-skip", onClick = onSkip}(opts.skipLabel);
        };
        const auto ctaEl = [&]() -> Nui::ElementRenderer
        {
            if (opts.ctaLabel.empty())
                return Nui::nil();
            return button{class_ = "snc-spotlight-cta", onClick = onCta}(opts.ctaLabel);
        };

        return div{
            class_ = "snc-spotlight-tooltip",
            "data-side"_attr = std::string{sideToString(g.chosenSide)},
            style = fmt::format("left:{}px;top:{}px", g.tooltipX, g.tooltipY),
        }(div{class_ = "snc-spotlight-tooltip-header"}(
              span{class_ = "snc-spotlight-step"}(stepText),
              button{
                  class_ = "snc-spotlight-close",
                  "aria-label"_attr = std::string{"Dismiss"},
                  onClick = onSkip,
              }(Utf8::cp(0x00D7)) // multiplication sign — used as close glyph
          ),
            titleEl(),
            bodyEl(),
            div{class_ = "snc-spotlight-footer"}(skipEl(), ctaEl()));
    }

    Nui::ElementRenderer SpotlightOverlay::Implementation::renderRoot()
    {
        using Nui::Elements::div;
        using Nui::Elements::fragment;
        using namespace Nui::Attributes;

        // The four sub-renderers all depend on `visible` + `geometry`, so a
        // single outer observe-and-generate is sufficient — and it's the
        // structure the parent element accepts (the variadic child overload
        // does not take ObservedValueCombinatorWithGenerator). The overlay
        // tree is small enough that rebuilding it per geometry change is
        // imperceptible.
        return div{
            class_ = observe(visible).generate(
                [this]() -> std::string
                {
                    return visible.value() ? std::string{"snc-spotlight-root snc-spotlight-root--visible"}
                                           : std::string{"snc-spotlight-root"};
                }
            ),
            "role"_attr = std::string{"dialog"},
            "aria-modal"_attr = std::string{"true"},
        }(observe(visible, geometry)
                .generate(
                    [this]() -> Nui::ElementRenderer
                    {
                        if (!visible.value())
                            return Nui::nil();
                        return fragment(renderBackdrop(), renderRing(), renderArrow(), renderTooltip());
                    }
                ));
    }

    // ── Public API ────────────────────────────────────────────────────

    SpotlightOverlay::SpotlightOverlay()
        : impl_{std::make_unique<Implementation>()}
    {
        impl_->instanceId = ++instanceCounter();
    }

    SpotlightOverlay::~SpotlightOverlay()
    {
        if (impl_)
            impl_->teardownListeners();
    }

    SpotlightOverlay::SpotlightOverlay(SpotlightOverlay&&) = default;
    SpotlightOverlay& SpotlightOverlay::operator=(SpotlightOverlay&&) = default;

    void SpotlightOverlay::show(SpotlightOptions options)
    {
        impl_->teardownListeners();
        impl_->currentOptions = std::move(options);

        // Lazy portal creation: the overlay's DOM only exists on first
        // show, then persists for the SpotlightOverlay's lifetime.
        // Subsequent show/hide cycles just toggle visibility — keeps
        // observer state and avoids re-binding listeners through a fresh
        // ElementRenderer.
        if (!impl_->portal)
            impl_->portal.emplace(impl_->renderRoot());

        impl_->visible = true;
        Implementation::Geometry reset{};
        impl_->geometry = reset;
        impl_->geometry.modifyNow();
        Nui::globalEventContext.executeActiveEventsImmediately();

        // Wait for the target element to mount before installing observers
        // and computing the first geometry. Settings dialog openings are
        // multi-frame, so retry up to ~one second worth of frames.
        // shared_ptr keeps the recursive lambda alive across rAF turns.
        constexpr int maxAttempts = 60;
        auto attempts = std::make_shared<int>(0);
        auto tick = std::make_shared<std::function<void()>>();
        *tick = [this, attempts, tick]()
        {
            if (!impl_->visible.value())
                return;
            const auto target = impl_->findTarget();
            if (!target.isNull() && !target.isUndefined())
            {
                impl_->installListeners();
                impl_->recompute();
                return;
            }
            if (++(*attempts) >= maxAttempts)
                return;
            Nui::val::global("requestAnimationFrame")(Nui::bind(
                [tick](Nui::val)
                {
                    (*tick)();
                },
                std::placeholders::_1
            ));
        };
        (*tick)();
    }

    void SpotlightOverlay::hide()
    {
        if (!impl_->visible.value())
            return;
        impl_->teardownListeners();
        impl_->visible = false;
        Implementation::Geometry reset{};
        impl_->geometry = reset;
        impl_->geometry.modifyNow();
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    bool SpotlightOverlay::isVisible() const
    {
        return impl_->visible.value();
    }
} // namespace ScriptNuiComponents
