#include <script-nui-components/carousel.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/val.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/api/abort_controller.hpp>

#include <ui5-sap-icons/icons/navigation-left-arrow.hpp>
#include <ui5-sap-icons/icons/navigation-right-arrow.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ScriptNuiComponents
{
    namespace
    {
        struct Card
        {
            Nui::ElementRenderer renderer;
        };

        // Minimum horizontal distance (px) for a touch movement to count as a swipe.
        constexpr double swipeThreshold = 60.0;
    }

    // ---------------------------------------------------------------------------
    // Implementation
    // ---------------------------------------------------------------------------
    struct Carousel::Implementation
    {
        std::shared_ptr<Nui::Observed<int>> page;

        Carousel::Factory factory;
        int itemCount;
        int cacheRadius;
        CarouselControlsPosition controlsPosition;
        bool disableSwipe;

        // One slot per logical card index. Slots outside the cache window stay
        // empty (nullptr) so the flex track keeps its horizontal positioning.
        std::vector<std::unique_ptr<Card>> cards;

        // Per-slot version counter. Bumping triggers re-render of just that
        // slot's reactive subtree -- so swapping a card in or out doesn't
        // disturb the smooth transform animation on the track.
        // Held by unique_ptr so the Observed addresses stay stable across
        // vector growth / move; lambdas capture by reference.
        std::vector<std::unique_ptr<Nui::Observed<int>>> slotVersions;

        Nui::Observed<std::vector<int>> slotIndices;

        bool prefersReducedMotion{false};

        // ---- Swipe / drag state (not observed -- purely imperative) -----------
        double touchStartX_{0.0};
        double touchStartY_{0.0};
        double touchLastX_{0.0};

        // Mouse drag -- mousemove/mouseup are registered on window so a drag that
        // leaves the track element still resolves correctly.
        bool mouseDown_{false};
        Nui::WebApi::AbortController mouseMoveAbort_{};
        Nui::WebApi::AbortController mouseUpAbort_{};

        Nui::ListenRemover<Nui::Observed<int>> pageListener;

        // ---- Helpers -----------------------------------------------------------

        std::pair<int, int> desiredWindow(int currentPage) const
        {
            if (itemCount <= 0)
                return {0, -1};
            return {
                std::max(0, currentPage - cacheRadius),
                std::min(itemCount - 1, currentPage + cacheRadius),
            };
        }

        std::unique_ptr<Card> makeCard(int index)
        {
            return std::make_unique<Card>(Card{factory(index)});
        }

        // Bumps a per-slot version counter, which triggers a re-render of just
        // that slot's reactive subtree (and only that slot's).
        void bumpSlot(int i)
        {
            if (i < 0 || i >= static_cast<int>(slotVersions.size()))
                return;
            *slotVersions[i] = **slotVersions[i] + 1;
        }

        void resizeSlotStorage()
        {
            auto const target = static_cast<std::size_t>(std::max(0, itemCount));
            cards.resize(target);
            slotVersions.resize(target);
            for (auto& slot : slotVersions)
            {
                if (!slot)
                    slot = std::make_unique<Nui::Observed<int>>(0);
            }
        }

        void rebuildSlotIndices()
        {
            slotIndices.clear();
            for (int i = 0; i < itemCount; ++i)
                slotIndices.push_back(i);
        }

        // ---- Cache window management -------------------------------------------
        // Eager-by-default: every slot inside [page-radius, page+radius] holds
        // a built Card; everything outside is nullptr (rendered as an empty
        // placeholder div so the flex track preserves its horizontal layout).

        void rebuildCache(int currentPage)
        {
            auto [lo, hi] = desiredWindow(currentPage);
            for (int i = 0; i < itemCount; ++i)
            {
                bool const inWindow = (i >= lo && i <= hi);
                bool const have = static_cast<bool>(cards[i]);
                if (inWindow && !have)
                {
                    cards[i] = makeCard(i);
                    bumpSlot(i);
                }
                else if (!inWindow && have)
                {
                    cards[i].reset();
                    bumpSlot(i);
                }
            }
        }

        // ---- Construction ------------------------------------------------------

        Implementation(
            std::shared_ptr<Nui::Observed<int>> sharedPage,
            Carousel::Factory f,
            int count,
            int radius,
            CarouselControlsPosition position,
            bool disableSwipe
        )
            : page{std::move(sharedPage)}
            , factory{std::move(f)}
            , itemCount{count}
            , cacheRadius{radius}
            , controlsPosition{position}
            , disableSwipe{disableSwipe}
            , pageListener{Nui::smartListen(*page, [](int) {})} // replaced below
        {
            auto mql = Nui::val::global("window").call<Nui::val>(
                "matchMedia", std::string{"(prefers-reduced-motion: reduce)"}
            );
            prefersReducedMotion = mql["matches"].as<bool>();

            resizeSlotStorage();
            rebuildSlotIndices();
            rebuildCache(**page);

            pageListener = Nui::smartListen(
                *page,
                [this](int newPage)
                {
                    newPage = std::clamp(newPage, 0, itemCount - 1);
                    if (**page != newPage)
                        *page = newPage;
                    rebuildCache(newPage);
                    Nui::globalEventContext.executeActiveEventsImmediately();
                }
            );
        }

        // ---- Navigation --------------------------------------------------------

        void goNext()
        {
            *page = std::min(**page + 1, itemCount - 1);
        }

        void goPrev()
        {
            *page = std::max(**page - 1, 0);
        }

        void goBy(int delta)
        {
            *page = std::clamp(**page + delta, 0, itemCount - 1);
        }

        // ---- Swipe handlers ----------------------------------------------------

        void onTouchStart(Nui::val event)
        {
            if (disableSwipe)
                return;

            auto touches = event["changedTouches"];
            if (touches["length"].as<int>() == 0)
                return;
            touchStartX_ = touches[0]["clientX"].as<double>();
            touchStartY_ = touches[0]["clientY"].as<double>();
            touchLastX_ = touchStartX_;
        }

        void onTouchMove(Nui::val event)
        {
            if (disableSwipe)
                return;

            auto touches = event["changedTouches"];
            if (touches["length"].as<int>() == 0)
                return;
            touchLastX_ = touches[0]["clientX"].as<double>();

            // Prevent the page from scrolling horizontally while swiping the carousel.
            double dx = touchLastX_ - touchStartX_;
            if (std::abs(dx) > swipeThreshold)
                event.call<void>("preventDefault");
        }

        void onTouchEnd(Nui::val event)
        {
            if (disableSwipe)
                return;

            auto touches = event["changedTouches"];
            if (touches["length"].as<int>() == 0)
                return;

            double endX = touches[0]["clientX"].as<double>();
            double endY = touches[0]["clientY"].as<double>();
            double dx = endX - touchStartX_;
            double dy = endY - touchStartY_;

            if (std::abs(dx) < swipeThreshold)
                return;
            if (std::abs(dy) > std::abs(dx))
                return;

            goBy(dx < 0 ? 1 : -1);
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        // ---- Mouse drag handlers -----------------------------------------------
        // mousemove and mouseup are bound to window on mousedown so the drag
        // resolves even if the pointer leaves the track div.

        void onMouseDown(Nui::val event)
        {
            if (disableSwipe)
                return;

            if (event["button"].as<int>() != 0)
                return; // left button only

            // Don't start a drag when the click originates on an interactive element.
            if (!event["target"].call<Nui::val>("closest", std::string{"button, a, input, select"}).isNull())
                return;

            mouseDown_ = true;
            touchStartX_ = event["clientX"].as<double>();
            touchStartY_ = event["clientY"].as<double>();
            touchLastX_ = touchStartX_;

            auto win = Nui::val::global("window");

            mouseMoveAbort_ = {};
            auto moveOptions = Nui::val::object();
            moveOptions.set("signal", mouseMoveAbort_.signal().val());
            win.call<void>(
                "addEventListener",
                std::string{"mousemove"},
                Nui::bind(
                    [this](Nui::val ev)
                    {
                        touchLastX_ = ev["clientX"].as<double>();
                    },
                    std::placeholders::_1
                ),
                moveOptions
            );

            mouseUpAbort_ = {};
            auto upOptions = Nui::val::object();
            upOptions.set("signal", mouseUpAbort_.signal().val());
            win.call<void>(
                "addEventListener",
                std::string{"mouseup"},
                Nui::bind(
                    [this](Nui::val ev)
                    {
                        mouseDown_ = false;
                        mouseMoveAbort_.abort();
                        mouseUpAbort_.abort();

                        // Don't commit a swipe if the mouse was released over an interactive element.
                        if (!ev["target"].call<Nui::val>("closest", std::string{"button, a, input, select"}).isNull())
                            return;

                        double endX = ev["clientX"].as<double>();
                        double endY = ev["clientY"].as<double>();
                        double dx = endX - touchStartX_;
                        double dy = endY - touchStartY_;
                        if (std::abs(dx) < swipeThreshold)
                            return;
                        if (std::abs(dy) > std::abs(dx))
                            return;

                        goBy(dx < 0 ? 1 : -1);
                        Nui::globalEventContext.executeActiveEventsImmediately();
                    },
                    std::placeholders::_1
                ),
                upOptions
            );
        }

        // ---- Render helpers ----------------------------------------------------

        Nui::ElementRenderer renderTrack()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            // Track translateX is driven by the current page; CSS handles the
            // smooth transition. The animated/static distinction is exposed
            // as a class so prefers-reduced-motion can disable the transition.
            std::string const trackClass = prefersReducedMotion
                ? std::string{"script-nui-carousel__track"}
                : std::string{"script-nui-carousel__track script-nui-carousel__track--animated"};

            return div{
                class_ = trackClass,
                style = Nui::observe(*page).generate(
                    [](int p) -> std::string
                    {
                        return fmt::format("transform: translateX(-{}%);", p * 100);
                    }
                ),
                "touchstart"_event =
                    [this](Nui::val event)
                {
                    onTouchStart(event);
                },
                "touchmove"_event =
                    [this](Nui::val event)
                {
                    onTouchMove(event);
                },
                "touchend"_event =
                    [this](Nui::val event)
                {
                    onTouchEnd(event);
                },
                "mousedown"_event =
                    [this](Nui::val event)
                {
                    onMouseDown(event);
                },
            }(Nui::range(slotIndices),
                [this](long long, int const& logicalIndex) -> Nui::ElementRenderer
                {
                    return div{
                        class_ = Nui::observe(*page).generate(
                            [logicalIndex](int currentPage) -> std::string
                            {
                                return logicalIndex == currentPage
                                    ? "script-nui-carousel__card script-nui-carousel__card--active"
                                    : "script-nui-carousel__card";
                            }
                        ),
                        "data-carousel-index"_attr = std::to_string(logicalIndex),
                    }(Nui::observe(*slotVersions[logicalIndex])
                            .generate(
                                [this, logicalIndex](int) -> Nui::ElementRenderer
                                {
                                    return (logicalIndex >= 0 && logicalIndex < static_cast<int>(cards.size()) &&
                                               cards[logicalIndex])
                                        ? cards[logicalIndex]->renderer
                                        : Nui::nil();
                                }
                            ));
                });
        }

        Nui::ElementRenderer renderPrev()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;

            return button{
                class_ = Nui::observe(*page).generate(
                    [](int p) -> std::string
                    {
                        return p == 0 ? "script-nui-carousel__prev script-nui-carousel__prev--disabled"
                                      : "script-nui-carousel__prev";
                    }
                ),
                "aria-label"_attr = std::string{"Previous slide"},
                onClick = [this](Nui::val)
                {
                    goPrev();
                },
            }(Ui5Icons::navigation_left_arrow());
        }

        Nui::ElementRenderer renderNext()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;

            return button{
                class_ = Nui::observe(*page).generate(
                    [this](int p) -> std::string
                    {
                        return p >= itemCount - 1 ? "script-nui-carousel__next script-nui-carousel__next--disabled"
                                                  : "script-nui-carousel__next";
                    }
                ),
                "aria-label"_attr = std::string{"Next slide"},
                onClick = [this](Nui::val)
                {
                    goNext();
                },
            }(Ui5Icons::navigation_right_arrow());
        }

        Nui::ElementRenderer renderCounter()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::span;

            // Pad to two digits for that "01 / 08" mono-font look.
            return span{class_ = "script-nui-carousel__counter"}(Nui::observe(*page).generate(
                [this](int p) -> std::string
                {
                    auto pad = [](int v) -> std::string
                    {
                        auto s = std::to_string(v);
                        return s.size() < 2 ? std::string(2 - s.size(), '0') + s : s;
                    };
                    return pad(p + 1) + " / " + pad(itemCount);
                }
            ));
        }

        Nui::ElementRenderer renderDots()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            return div{
                class_ = "script-nui-carousel__dots"
            }(Nui::range(slotIndices),
                [this](long long, int const& i) -> Nui::ElementRenderer
                {
                    return button{
                        class_ = Nui::observe(*page).generate(
                            [i](int p) -> std::string
                            {
                                return p == i ? "script-nui-carousel__dot script-nui-carousel__dot--active"
                                              : "script-nui-carousel__dot";
                            }
                        ),
                        "aria-label"_attr = std::string{"Go to slide "} + std::to_string(i + 1),
                        onClick = [this, i](Nui::val)
                        {
                            *page = i;
                            Nui::globalEventContext.executeActiveEventsImmediately();
                        },
                    }();
                });
        }

        // Legacy controls bar used for the Sides layout: prev + indicator + next.
        Nui::ElementRenderer renderControls()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            return div{
                class_ = "script-nui-carousel__controls"
            }(renderPrev(),
                span{class_ = "script-nui-carousel__indicator"}(Nui::observe(*page).generate(
                    [this](int p) -> Nui::ElementRenderer
                    {
                        using namespace Nui::Elements;
                        return span{}(std::to_string(p + 1) + " / " + std::to_string(itemCount));
                    }
                )),
                renderNext());
        }

        // Modern controls bar used for Top/Bottom: dots strip on the left,
        // arrows + counter pill on the right.
        Nui::ElementRenderer renderControlsBar()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            return div{
                class_ = "script-nui-carousel__controls"
            }(renderDots(), div{class_ = "script-nui-carousel__arrows"}(renderPrev(), renderCounter(), renderNext()));
        }

        // ---- Rendering ---------------------------------------------------------

        Nui::ElementRenderer render(std::vector<Nui::Attribute> additionalAttributes)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            // Reflect the controls position as a modifier class so CSS can adjust
            // layout without needing separate wrapper structures.
            auto positionClass = [this]() -> std::string
            {
                switch (controlsPosition)
                {
                    case CarouselControlsPosition::Top:
                        return "script-nui-carousel script-nui-carousel--controls-top";
                    case CarouselControlsPosition::Bottom:
                        return "script-nui-carousel script-nui-carousel--controls-bottom";
                    case CarouselControlsPosition::Sides:
                    default:
                        return "script-nui-carousel script-nui-carousel--controls-sides";
                }
            }();

            std::vector<Nui::Attribute> baseAttrs;
            baseAttrs.push_back(class_ = positionClass);
            auto mergedAttrs = Nui::mergeAttributes(std::move(baseAttrs), std::move(additionalAttributes));

            // Top/Bottom: controls bar above or below the track (flex-column).
            // Sides: buttons flank the track, indicator floats below (or can be
            //        positioned via CSS).
            if (controlsPosition == CarouselControlsPosition::Sides)
            {
                return div{std::move(mergedAttrs)}(div{class_ = "script-nui-carousel__body"}(
                    renderControls(), // contains only prev button when sides
                    renderTrack(),
                    renderControls() // contains only next button when sides -- CSS hides the other
                ));
            }

            // Top or Bottom: glass frame around the track, modern controls bar
            // (dots + arrows + counter) above or below.
            using Nui::Elements::div;
            auto framed = div{class_ = "script-nui-carousel__frame"}(renderTrack());

            if (controlsPosition == CarouselControlsPosition::Top)
            {
                return div{std::move(mergedAttrs)}(renderControlsBar(), std::move(framed));
            }

            // Bottom (default)
            return div{std::move(mergedAttrs)}(std::move(framed), renderControlsBar());
        }
    };

    // ---------------------------------------------------------------------------
    // Carousel public API
    // ---------------------------------------------------------------------------

    Carousel::Carousel(
        std::shared_ptr<Nui::Observed<int>> page,
        Factory factory,
        int itemCount,
        int cacheRadius,
        CarouselControlsPosition controlsPosition,
        bool disableSwipe
    )
        : impl_{std::make_unique<Implementation>(
              std::move(page),
              std::move(factory),
              itemCount,
              cacheRadius,
              controlsPosition,
              disableSwipe
          )}
    {}

    Carousel::~Carousel() = default;
    Carousel::Carousel(Carousel&&) = default;
    Carousel& Carousel::operator=(Carousel&&) = default;

    void Carousel::setItemCount(int count)
    {
        impl_->itemCount = count;
        int currentPage = **(impl_->page);
        int clamped = std::clamp(currentPage, 0, std::max(0, count - 1));
        if (clamped != currentPage)
            *(impl_->page) = clamped;
        impl_->resizeSlotStorage();
        impl_->rebuildSlotIndices();
        impl_->rebuildCache(**(impl_->page));
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    Nui::ElementRenderer Carousel::operator()(std::vector<Nui::Attribute> additionalAttributes)
    {
        return impl_->render(std::move(additionalAttributes));
    }
}