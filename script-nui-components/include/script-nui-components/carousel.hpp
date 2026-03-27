#pragma once
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp> // Nui::Attribute
#include <nui/event_system/observed_value.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace ScriptNuiComponents
{
    /// A memoizing, virtualised carousel component.
    ///
    /// Ownership model
    /// ---------------
    /// - `page`  is a shared_ptr<Observed<int>> so both the caller and the component
    ///   can read and write the current page, and the component can safely listen to
    ///   it via smartListen without lifetime hazards.
    /// - `itemCount` is the initial capacity; call setItemCount() later to change it.
    /// - `factory` is called at most once per index; results are cached in a deque of
    ///   unique_ptr<Card> to avoid pointer instability on deque growth.
    ///
    /// Cache window
    /// ------------
    /// Only [page - cacheRadius, page + cacheRadius] slots are kept alive.  Cards
    /// outside that window are destroyed; re-entering the window recreates them via
    /// `factory`.  Default radius is 5 (11 cards total).
    ///
    /// Reduced-motion
    /// --------------
    /// prefers-reduced-motion is sampled once at construction time.  If it is active
    /// no CSS transition class is applied and the slide animation is suppressed.
    class Carousel
    {
      public:
        using Factory = std::function<Nui::ElementRenderer(int index)>;

        /// @param page        Shared, externally-owned page counter.
        /// @param factory     Called once per index to produce a card; result is cached.
        /// @param itemCount   Initial number of items in the carousel.
        /// @param cacheRadius How many cards on each side of the current page to keep
        ///                    rendered.  Cards beyond this radius are evicted and
        ///                    regenerated on demand.  0 = only current card cached.
        explicit Carousel(
            std::shared_ptr<Nui::Observed<int>> page,
            Factory factory,
            int itemCount,
            int cacheRadius = 5
        );

        ~Carousel();
        Carousel(Carousel const&) = delete;
        Carousel(Carousel&&);
        Carousel& operator=(Carousel const&) = delete;
        Carousel& operator=(Carousel&&);

        /// Change the total number of items.  The current page is clamped if needed.
        void setItemCount(int count);

        /// Render the carousel.
        /// @param additionalAttributes  Merged onto the outermost container element.
        Nui::ElementRenderer operator()(std::vector<Nui::Attribute> additionalAttributes = {});

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}