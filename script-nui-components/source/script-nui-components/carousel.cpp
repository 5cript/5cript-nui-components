#include <script-nui-components/carousel.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/val.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/api/abort_controller.hpp>

#include <ui5-sap-icons/icons/navigation-left-arrow.hpp>
#include <ui5-sap-icons/icons/navigation-right-arrow.hpp>

#include <algorithm>
#include <cmath>
#include <deque>
#include <memory>
#include <string>

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
        int windowLo{0};
        CarouselControlsPosition controlsPosition;
        bool disableSwipe;

        Nui::Observed<std::deque<std::unique_ptr<Card>>> cards;

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

        int windowHi() const
        {
            return windowLo + static_cast<int>(cards->size()) - 1;
        }

        std::pair<int, int> desiredWindow(int currentPage) const
        {
            return {
                std::max(0, currentPage - cacheRadius),
                std::min(itemCount - 1, currentPage + cacheRadius),
            };
        }

        std::unique_ptr<Card> makeCard(int index)
        {
            return std::make_unique<Card>(Card{factory(index)});
        }

        // ---- Window management -------------------------------------------------

        void rebuildWindow(int currentPage)
        {
            auto [lo, hi] = desiredWindow(currentPage);
            windowLo = lo;
            auto proxy = cards.modify();
            proxy.value().clear();
            for (int i = lo; i <= hi; ++i)
                proxy.value().push_back(makeCard(i));
        }

        void slideWindow(int newPage)
        {
            auto [newLo, newHi] = desiredWindow(newPage);

            if (cards->empty() || newHi < windowLo || newLo > windowHi())
            {
                rebuildWindow(newPage);
                return;
            }

            while (windowHi() > newHi)
                cards.pop_back();
            while (windowHi() < newHi)
                cards.push_back(makeCard(windowHi() + 1));

            while (windowLo < newLo)
            {
                cards.pop_front();
                ++windowLo;
            }
            while (windowLo > newLo)
            {
                --windowLo;
                cards.push_front(makeCard(windowLo));
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

            rebuildWindow(**page);

            pageListener = Nui::smartListen(
                *page,
                [this](int newPage)
                {
                    newPage = std::clamp(newPage, 0, itemCount - 1);
                    if (**page != newPage)
                        *page = newPage;
                    slideWindow(newPage);
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

            return div{
                class_ = "script-nui-carousel__track",
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
            }(Nui::range(cards),
                [this](long long i, std::unique_ptr<Card> const& card) -> Nui::ElementRenderer
                {
                    int logicalIndex = windowLo + static_cast<int>(i);
                    return div{
                        class_ = Nui::observe(*page).generate(
                            [this, logicalIndex](int currentPage) -> std::string
                            {
                                std::string cls = "script-nui-carousel__card";
                                if (!prefersReducedMotion)
                                    cls += " script-nui-carousel__animated";
                                if (logicalIndex == currentPage)
                                    cls += " script-nui-carousel__card--active";
                                return cls;
                            }
                        ),
                        "data-carousel-index"_attr = std::to_string(logicalIndex),
                    }(card->renderer);
                });
        }

        Nui::ElementRenderer renderControls()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            return div{class_ = "script-nui-carousel__controls"}(
                button{
                    class_ = Nui::observe(*page).generate(
                        [](int p) -> std::string
                        {
                            return p == 0 ? "script-nui-carousel__prev script-nui-carousel__prev--disabled"
                                          : "script-nui-carousel__prev";
                        }
                    ),
                    onClick =
                        [this](Nui::val)
                    {
                        goPrev();
                    },
                }(Ui5Icons::navigation_left_arrow()),
                span{class_ = "script-nui-carousel__indicator"}(Nui::observe(*page).generate(
                    [this](int p) -> Nui::ElementRenderer
                    {
                        using namespace Nui::Elements;
                        return span{}(std::to_string(p + 1) + " / " + std::to_string(itemCount));
                    }
                )),
                button{
                    class_ = Nui::observe(*page).generate(
                        [this](int p) -> std::string
                        {
                            return p >= itemCount - 1 ? "script-nui-carousel__next script-nui-carousel__next--disabled"
                                                      : "script-nui-carousel__next";
                        }
                    ),
                    onClick = [this](Nui::val)
                    {
                        goNext();
                    },
                }(Ui5Icons::navigation_right_arrow())
            );
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

            // Top or Bottom: stacked layout
            if (controlsPosition == CarouselControlsPosition::Top)
            {
                return div{std::move(mergedAttrs)}(renderControls(), renderTrack());
            }

            // Bottom (default)
            return div{std::move(mergedAttrs)}(renderTrack(), renderControls());
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
        impl_->rebuildWindow(**(impl_->page));
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    Nui::ElementRenderer Carousel::operator()(std::vector<Nui::Attribute> additionalAttributes)
    {
        return impl_->render(std::move(additionalAttributes));
    }
}