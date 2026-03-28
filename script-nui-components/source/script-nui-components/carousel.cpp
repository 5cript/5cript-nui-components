#include <script-nui-components/carousel.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/val.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>

#include <ui5-sap-icons/icons/navigation-left-arrow.hpp>
#include <ui5-sap-icons/icons/navigation-right-arrow.hpp>

#include <algorithm>
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
    }

    // ---------------------------------------------------------------------------
    // Implementation
    // ---------------------------------------------------------------------------
    // `cards` is the direct range source for Nui::range.  It always covers exactly
    // [windowLo, windowLo + cards->size()) clamped to [0, itemCount).
    // Navigating by ±1 uses pop_front/push_back (or the reverse) so Nui only
    // patches the single added/removed DOM node.  Larger jumps use a full rebuild.
    // ---------------------------------------------------------------------------
    struct Carousel::Implementation
    {

        std::shared_ptr<Nui::Observed<int>> page;

        Carousel::Factory factory;
        int itemCount;
        int cacheRadius;
        int windowLo{0}; // logical index of cards->front()

        Nui::Observed<std::deque<std::unique_ptr<Card>>> cards;

        bool prefersReducedMotion{false};

        Nui::ListenRemover<Nui::Observed<int>> pageListener;

        // ---- Helpers -----------------------------------------------------------

        int windowHi() const
        {
            return windowLo + static_cast<int>(cards->size()) - 1;
        }

        std::pair<int, int> desiredWindow(int currentPage) const
        {
            return {std::max(0, currentPage - cacheRadius), std::min(itemCount - 1, currentPage + cacheRadius)};
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

        Implementation(std::shared_ptr<Nui::Observed<int>> sharedPage, Carousel::Factory f, int count, int radius)
            : page{std::move(sharedPage)}
            , factory{std::move(f)}
            , itemCount{count}
            , cacheRadius{radius}
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

        void goNext()
        {
            *page = std::min(**page + 1, itemCount - 1);
        }
        void goPrev()
        {
            *page = std::max(**page - 1, 0);
        }

        // ---- Rendering ---------------------------------------------------------

        Nui::ElementRenderer render(std::vector<Nui::Attribute> additionalAttributes)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            std::vector<Nui::Attribute> baseAttrs;
            baseAttrs.push_back(class_ = "script-nui-carousel");
            auto mergedAttrs = Nui::mergeAttributes(std::move(baseAttrs), std::move(additionalAttributes));

            return div{std::move(mergedAttrs)}(
                div{
                    class_ = "script-nui-carousel__track"
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
                            "data-carousel-index"_attr = std::to_string(logicalIndex)
                        }(card->renderer);
                    }),
                div{class_ = "script-nui-carousel__controls"}(
                    button{
                        class_ = Nui::observe(*page).generate(
                            [](int p) -> std::string
                            {
                                return p == 0 ? "script-nui-carousel__prev carousel__prev--disabled"
                                              : "script-nui-carousel__prev";
                            }
                        ),
                        onClick =
                            [this](Nui::val)
                        {
                            goPrev();
                        }
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
                                return p >= itemCount - 1 ? "script-nui-carousel__next carousel__next--disabled"
                                                          : "script-nui-carousel__next";
                            }
                        ),
                        onClick = [this](Nui::val)
                        {
                            goNext();
                        }
                    }(Ui5Icons::navigation_right_arrow())
                )
            );
        }
    };

    // ---------------------------------------------------------------------------
    // Carousel public API
    // ---------------------------------------------------------------------------

    Carousel::Carousel(std::shared_ptr<Nui::Observed<int>> page, Factory factory, int itemCount, int cacheRadius)
        : impl_{std::make_unique<Implementation>(std::move(page), std::move(factory), itemCount, cacheRadius)}
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