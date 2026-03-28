#pragma once
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/event_system/observed_value.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace ScriptNuiComponents
{
    /// Where the prev/next arrow buttons are rendered relative to the card track.
    ///
    ///   Sides  -- buttons flank the track on the left and right (default)
    ///   Top    -- prev/indicator/next bar sits above the track
    ///   Bottom -- prev/indicator/next bar sits below the track
    enum class CarouselControlsPosition
    {
        Sides,
        Top,
        Bottom,
    };

    /// A memoizing, virtualised carousel component.
    ///
    /// Ownership model
    /// ---------------
    /// - `page` is a shared_ptr<Observed<int>> so both the caller and the component
    ///   can read and write the current page, and the component can safely listen to
    ///   it via smartListen without lifetime hazards.
    /// - `itemCount` is the initial capacity; call setItemCount() later to change it.
    /// - `factory` is called at most once per index; results are cached in a deque of
    ///   unique_ptr<Card>.
    ///
    /// Cache window
    /// ------------
    /// Only [page - cacheRadius, page + cacheRadius] slots are kept alive. Cards
    /// outside that window are destroyed; re-entering the window recreates them via
    /// `factory`. Default radius is 5 (11 cards total).
    ///
    /// Reduced-motion
    /// --------------
    /// prefers-reduced-motion is sampled once at construction time. If it is active
    /// no CSS animation class is applied.
    ///
    /// Swipe
    /// -----
    /// Touch swipe left/right navigates the carousel. Velocity is tracked so a fast
    /// flick can skip multiple pages proportional to swipe speed.
    class Carousel
    {
      public:
        using Factory = std::function<Nui::ElementRenderer(int index)>;

        /// @param page             Shared, externally-owned page counter.
        /// @param factory          Called once per index to produce a card; result is cached.
        /// @param itemCount        Initial number of items in the carousel.
        /// @param cacheRadius      Cards kept alive on each side of the current page.
        /// @param controlsPosition Where to render the prev/next controls.
        /// @param disableSwipe     If true, touch swipe navigation is disabled.
        explicit Carousel(
            std::shared_ptr<Nui::Observed<int>> page,
            Factory factory,
            int itemCount,
            int cacheRadius = 5,
            CarouselControlsPosition controlsPosition = CarouselControlsPosition::Bottom,
            bool disableSwipe = false
        );

        ~Carousel();
        Carousel(Carousel const&) = delete;
        Carousel(Carousel&&);
        Carousel& operator=(Carousel const&) = delete;
        Carousel& operator=(Carousel&&);

        /// Change the total number of items. The current page is clamped if needed.
        void setItemCount(int count);

        /// Render the carousel.
        /// @param additionalAttributes  Merged onto the outermost container element.
        Nui::ElementRenderer operator()(std::vector<Nui::Attribute> additionalAttributes = {});

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}