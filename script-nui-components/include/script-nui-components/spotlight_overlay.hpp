#pragma once

#include <nui/frontend/element_renderer.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-spotlight-overlay)
#    include "../../../styles/spotlight_overlay.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /** @brief Side of the target where the tooltip card is placed.
     *         `Auto` picks the side with the most free space at render time. */
    enum class SpotlightSide
    {
        Auto,
        Top,
        Right,
        Bottom,
        Left,
    };

    /** @brief Step indicator rendered as `STEP n OF m` in the tooltip header. */
    struct SpotlightStepCounter
    {
        unsigned current{1};
        unsigned total{1};
    };

    /** @brief Configuration passed to `SpotlightOverlay::show`. All callbacks
     *         are invoked from event-context handlers; they may freely mutate
     *         `Observed<>` values. */
    struct SpotlightOptions
    {
        /** @brief DOM id of the element to highlight. The overlay polls
         *         `document.getElementById` so the target may mount after
         *         `show()` is called. */
        std::string targetElementId;
        /** @brief Tooltip title (rendered as `<h3>`). May be empty. */
        std::string title{};
        /** @brief Tooltip body paragraph. May be empty. */
        std::string bodyText{};
        /** @brief Optional step indicator. */
        std::optional<SpotlightStepCounter> stepCounter{};
        /** @brief Preferred side; falls back to `Auto` semantics if the
         *         preferred side cannot fit the tooltip. */
        SpotlightSide preferredSide{SpotlightSide::Auto};
        /** @brief Padding (px) added around the target rect for the cutout
         *         and the highlight ring. */
        double cutoutPadding{1.0};
        /** @brief Gap (px) between the target rect and the tooltip card. */
        double tooltipGap{36.0};
        /** @brief Backdrop dim opacity, 0..1. */
        double dimOpacity{0.72};
        /** @brief Primary CTA label ("Next" / "Finish"). Empty hides the CTA. */
        std::string ctaLabel{};
        /** @brief Skip button label. Empty hides the button. */
        std::string skipLabel{};
        /** @brief Whether clicking the dim backdrop dismisses. */
        bool dismissOnBackdropClick{true};
        /** @brief Whether the Esc key dismisses. */
        bool dismissOnEsc{true};
        /** @brief Invoked when the user clicks the CTA. */
        std::function<void()> onAdvance{};
        /** @brief Invoked when the user dismisses (Skip, close, backdrop, Esc). */
        std::function<void()> onDismiss{};
    };

    /** @brief Coachmark spotlight overlay: dims the viewport, cuts a
     *         rounded-rect hole around a target element, draws a green
     *         highlight ring + chevron arrow, and shows a positioned
     *         tooltip card. Repositions on window resize, target rect
     *         changes (`ResizeObserver`), and DOM mutations
     *         (`MutationObserver`). The instance mounts itself into
     *         `document.body` via a `Nui::BodyPortal` on first `show`;
     *         consumers do not need to embed a renderer in their tree. */
    class SpotlightOverlay
    {
      public:
        SpotlightOverlay();
        ~SpotlightOverlay();
        SpotlightOverlay(SpotlightOverlay const&) = delete;
        SpotlightOverlay(SpotlightOverlay&&);
        SpotlightOverlay& operator=(SpotlightOverlay const&) = delete;
        SpotlightOverlay& operator=(SpotlightOverlay&&);

        /** @brief Show with the given options. Replaces any prior options. */
        void show(SpotlightOptions options);

        /** @brief Hide. Idempotent. */
        void hide();

        /** @brief Whether the spotlight is currently visible. */
        bool isVisible() const;

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
} // namespace ScriptNuiComponents
