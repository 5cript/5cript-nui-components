#include <script-nui-components/carousel.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/val.hpp>
#include <nui/frontend/utility/merge_attributes.hpp> // Nui::mergeAttributes

#include <algorithm>
#include <deque>
#include <memory>
#include <string>

namespace ScriptNuiComponents
{
    // ---------------------------------------------------------------------------
    // Internal card wrapper
    // ---------------------------------------------------------------------------
    // Each slot in the deque is either:
    //   - nullptr          → outside the cache window, not yet created or evicted
    //   - non-null         → cached ElementRenderer produced by the factory
    //
    // We box ElementRenderer in a struct so that the deque stores unique_ptr<Card>
    // and pointer addresses of *Card stay stable even as the deque grows at either
    // end (push_front / push_back).  unique_ptr also makes "evict" trivially cheap:
    // just reset() the pointer.
    // ---------------------------------------------------------------------------
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
    struct Carousel::Implementation
    {

        // ---- Shared external page state ----------------------------------------
        std::shared_ptr<Nui::Observed<int>> page;

        // ---- Factory and cache -------------------------------------------------
        Carousel::Factory factory;
        int itemCount;
        int cacheRadius;

        // The deque covers indices [dequeOffset, dequeOffset + deque.size()).
        // A nullptr slot means "not cached"; a valid unique_ptr means cached.
        std::deque<std::unique_ptr<Card>> cards;
        int dequeOffset{0}; // logical index of cards.front()

        // ---- Internal observed list that drives range rendering ----------------
        // We keep a flat Observed<std::deque<unique_ptr<Card>>> so that push_back /
        // pop_front triggers minimal DOM patching through the Nui range API.
        // The range renderer just reads the Card* pointer and returns its cached
        // ElementRenderer; if the pointer is null it returns nil() as a placeholder
        // for a slot that is outside the visible window but inside the deque span.
        //
        // NOTE: We don't actually expose the full deque to the range -- we only
        // show the active window.  The range is driven by a separate small
        // Observed<std::vector<int>> of *logical indices* in the active window.
        // Each element is rendered by fetching (or creating) the card at that index.
        Nui::Observed<std::vector<int>> visibleIndices;

        // ---- Reduced-motion flag (sampled once at construction) -----------------
        bool prefersReducedMotion{false};

        // ---- Listener for external page changes --------------------------------
        Nui::ListenRemover<Nui::Observed<int>> pageListener;

        // ---- Construction helpers ----------------------------------------------

        Implementation(std::shared_ptr<Nui::Observed<int>> sharedPage, Carousel::Factory f, int count, int radius)
            : page{std::move(sharedPage)}
            , factory{std::move(f)}
            , itemCount{count}
            , cacheRadius{radius}
            , pageListener{// Populated at the end of the constructor body after all members
                  // are initialised; we use a two-step init below.
                  // Assign a dummy to satisfy the member initialiser list ordering
                  // requirement -- we'll overwrite in the ctor body.
                  Nui::smartListen(*page, [](int) {})
              }
        {
            // Sample prefers-reduced-motion
            {
                auto mql = Nui::val::global("window").call<Nui::val>(
                    "matchMedia", std::string{"(prefers-reduced-motion: reduce)"}
                );
                prefersReducedMotion = mql["matches"].as<bool>();
            }

            // Initialise the cache for the starting page
            rebuildCache(**page);

            // Replace the dummy listener with the real one
            pageListener = Nui::smartListen(
                *page,
                [this](int newPage)
                {
                    onPageChanged(newPage);
                }
            );
        }

        // ---- Cache management --------------------------------------------------

        /// Ensure the card at `index` exists in the deque and has a cached Card.
        /// Returns nullptr if index is out of [0, itemCount).
        Card* ensureCard(int index)
        {
            if (index < 0 || index >= itemCount)
                return nullptr;

            // Extend the deque to include `index` if needed
            if (cards.empty())
            {
                cards.push_back(nullptr);
                dequeOffset = index;
            }
            while (index < dequeOffset)
            {
                cards.push_front(nullptr);
                --dequeOffset;
            }
            while (index >= dequeOffset + static_cast<int>(cards.size()))
            {
                cards.push_back(nullptr);
            }

            int slot = index - dequeOffset;
            if (!cards[slot])
            {
                cards[slot] = std::make_unique<Card>(Card{factory(index)});
            }
            return cards[slot].get();
        }

        /// Evict cards that are outside [page - cacheRadius, page + cacheRadius].
        /// Shrinks the deque from the front and back to free memory.
        void evictOutsideWindow(int currentPage)
        {
            if (cards.empty())
                return;

            int windowLo = std::max(0, currentPage - cacheRadius);
            int windowHi = std::min(itemCount - 1, currentPage + cacheRadius);

            // Evict from the back (indices beyond windowHi)
            while (!cards.empty())
            {
                int lastIdx = dequeOffset + static_cast<int>(cards.size()) - 1;
                if (lastIdx > windowHi)
                {
                    cards.pop_back();
                }
                else
                {
                    break;
                }
            }

            // Evict from the front (indices below windowLo)
            while (!cards.empty() && dequeOffset < windowLo)
            {
                cards.pop_front();
                ++dequeOffset;
            }
        }

        /// Rebuild the visible-index list for the given page.
        void rebuildCache(int currentPage)
        {
            evictOutsideWindow(currentPage);

            // Pre-populate the entire window eagerly so the range is stable
            int windowLo = std::max(0, currentPage - cacheRadius);
            int windowHi = std::min(itemCount - 1, currentPage + cacheRadius);

            std::vector<int> indices;
            indices.reserve(windowHi - windowLo + 1);
            for (int i = windowLo; i <= windowHi; ++i)
            {
                ensureCard(i);
                indices.push_back(i);
            }
            visibleIndices = std::move(indices);
        }

        // ---- Page-change handler -----------------------------------------------

        void onPageChanged(int newPage)
        {
            newPage = std::clamp(newPage, 0, itemCount - 1);
            if (**page != newPage)
                *page = newPage; // clamp external value if needed

            rebuildCache(newPage);
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        // ---- Navigation helpers ------------------------------------------------

        void goNext()
        {
            int next = std::min(**page + 1, itemCount - 1);
            *page = next;
            // The smartListen callback fires and calls onPageChanged
        }

        void goPrev()
        {
            int prev = std::max(**page - 1, 0);
            *page = prev;
        }

        // ---- Rendering ---------------------------------------------------------

        Nui::ElementRenderer render(std::vector<Nui::Attribute> additionalAttributes)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            // Animation class string
            auto animClass = [this](std::string base) -> std::string
            {
                if (!prefersReducedMotion)
                    base += " carousel__animated";
                return base;
            };

            // --- Outer container -----------------------------------------------
            std::vector<Nui::Attribute> baseAttrs;
            baseAttrs.push_back(class_ = "carousel");

            auto mergedAttrs = Nui::mergeAttributes(std::move(baseAttrs), std::move(additionalAttributes));

            // --- Card track: only the current card is visible via CSS ----------
            // The range renders all cached cards; CSS hides non-active ones.
            // The active card is identified by its index matching *page.
            //
            // Outer structure:
            //   div.carousel                   ← mergedAttrs applied here
            //     div.carousel__track          ← scrolling container
            //       [card divs via range]
            //     div.carousel__controls
            //       button.carousel__prev
            //       span.carousel__indicator
            //       button.carousel__next

            return div{std::move(mergedAttrs)}(
                div{
                    class_ = "carousel__track"
                }(Nui::range(visibleIndices),
                    [this, animClass](long long /*i*/, int const& logicalIndex) -> Nui::ElementRenderer
                    {
                        Card* card = ensureCard(logicalIndex);
                        if (!card)
                            return Nui::nil();

                        // Active card gets carousel__card--active; CSS controls
                        // visibility/position.  We use observe(*page) so the
                        // class reacts to page changes without re-running the
                        // factory.
                        return div{
                            class_ = Nui::observe(*page).generate(
                                [logicalIndex, &animClass](int currentPage) -> std::string
                                {
                                    std::string cls = animClass("carousel__card");
                                    if (logicalIndex == currentPage)
                                        cls += " carousel__card--active";
                                    return cls;
                                }
                            ),
                            "data-carousel-index"_attr = std::to_string(logicalIndex)
                        }(card->renderer);
                    }),
                div{class_ = "carousel__controls"}(
                    button{
                        class_ = Nui::observe(*page).generate(
                            [](int p) -> std::string
                            {
                                return p == 0 ? "carousel__prev carousel__prev--disabled" : "carousel__prev";
                            }
                        ),
                        onClick =
                            [this](Nui::val)
                        {
                            goPrev();
                        }
                    }("‹"),
                    span{class_ = "carousel__indicator"}(Nui::observe(*page).generate(
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
                                return p >= itemCount - 1 ? "carousel__next carousel__next--disabled"
                                                          : "carousel__next";
                            }
                        ),
                        onClick = [this](Nui::val)
                        {
                            goNext();
                        }
                    }("›")
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
        // Clamp current page into the new range
        int currentPage = **(impl_->page);
        int clamped = std::clamp(currentPage, 0, std::max(0, count - 1));
        if (clamped != currentPage)
            *(impl_->page) = clamped;

        // Evict cards that are now out of bounds
        while (!impl_->cards.empty())
        {
            int lastIdx = impl_->dequeOffset + static_cast<int>(impl_->cards.size()) - 1;
            if (lastIdx >= count)
            {
                impl_->cards.pop_back();
            }
            else
            {
                break;
            }
        }
        impl_->rebuildCache(**(impl_->page));
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    Nui::ElementRenderer Carousel::operator()(std::vector<Nui::Attribute> additionalAttributes)
    {
        return impl_->render(std::move(additionalAttributes));
    }
}