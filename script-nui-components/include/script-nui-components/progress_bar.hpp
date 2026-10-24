#pragma once

#include <script-nui-components/progress_color.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-progress-bar)
#    include "../../../styles/progress_bar.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /**
     * @brief What a progress bar writes onto itself.
     */
    enum class ProgressBarTextMode
    {
        /**
         * @brief No text.
         */
        None,

        /**
         * @brief The percentage, ProgressBarTexts::percent.
         */
        Percent,

        /**
         * @brief Current and maximum as plain numbers plus the percentage, ProgressBarTexts::counts.
         */
        Counts,

        /**
         * @brief Current and maximum as byte sizes plus the percentage, ProgressBarTexts::bytes.
         */
        Bytes,
    };

    /**
     * @brief The built-in texts of a progress bar as fmt format strings. English defaults.
     */
    struct ProgressBarTexts
    {
        /**
         * @brief Receives the percentage as a double from 0 to 100.
         */
        std::string percent = "{:.1f}%";

        /**
         * @brief Receives current, maximum and the percentage.
         */
        std::string counts = "{} / {} ({:.1f}%)";

        /**
         * @brief Receives the formatted current, the formatted maximum and the percentage. Both sizes use the unit
         *        of the maximum: "{}  B" below 1000, else "{:.2f} KB", "MB", "GB" or "TB" in steps of 1024.
         */
        std::string bytes = "{} - {} ({:.1f}%)";
    };

    /**
     * @brief The values a progress bar shows, handed to ProgressBarOptions::formatText.
     */
    struct ProgressBarValues
    {
        /**
         * @brief The current value as given, not clamped.
         */
        std::int64_t current = 0;

        /**
         * @brief The lower bound.
         */
        std::int64_t minimum = 0;

        /**
         * @brief The upper bound.
         */
        std::int64_t maximum = 0;

        /**
         * @brief Progress from 0 to 1: (current - minimum) / (maximum - minimum) with current clamped. 0 when maximum
         *        is 0 (1 with zeroAsComplete), 1 when minimum equals a nonzero maximum.
         */
        double fraction = 0.0;

        /**
         * @brief Whether current reached maximum. The shine stops and the hue is 120 then; a bar from 0 to 0 is
         *        complete but only fills with zeroAsComplete.
         */
        bool complete = false;
    };

    /**
     * @brief Options of the reactive progress bar.
     */
    struct ProgressBarOptions
    {
        /**
         * @brief The current value. A null pointer counts as 0.
         */
        std::shared_ptr<Nui::Observed<std::int64_t>> current = {};

        /**
         * @brief The upper bound. A null pointer counts as 0.
         */
        std::shared_ptr<Nui::Observed<std::int64_t>> maximum = {};

        /**
         * @brief The lower bound, see ProgressBarValues::fraction.
         */
        std::int64_t minimum = 0;

        /**
         * @brief What the bar writes onto itself.
         */
        ProgressBarTextMode textMode = ProgressBarTextMode::Percent;

        /**
         * @brief Hue colors the fill by progress; Theme uses --script-nui-components-progress-bar-theme.
         */
        ProgressColorMode colorMode = ProgressColorMode::Hue;

        /**
         * @brief Fills a bar whose maximum is 0 completely (100 %) instead of leaving it empty.
         */
        bool zeroAsComplete = false;

        /**
         * @brief Runs the shine animation while the bar is not complete. False renders no shine part.
         */
        bool shine = true;

        /**
         * @brief CSS height set on the bar. Unset uses --script-nui-components-progress-bar-height.
         */
        std::optional<std::string> height = std::nullopt;

        /**
         * @brief Replaces the built-in text of the Percent, Counts and Bytes modes. None still shows no text.
         */
        std::function<std::string(ProgressBarValues const&)> formatText = {};

        /**
         * @brief The built-in texts.
         */
        ProgressBarTexts texts = {};

        /**
         * @brief Extra attributes merged onto the bar root.
         */
        std::vector<Nui::Attribute> attributes = {};
    };

    /**
     * @brief A horizontal progress bar bound to shared observed values.
     *
     * Markup: div.script-nui-progress-bar[role=progressbar][data-color-mode][data-complete] holding, in this order,
     * div.script-nui-progress-bar-fill, div.script-nui-progress-bar-text and div.script-nui-progress-bar-shine.
     * data-color-mode is "hue" or "theme"; data-complete is present once complete. The shine part is missing when
     * ProgressBarOptions::shine is false and hidden while complete or under prefers-reduced-motion. aria-valuenow
     * carries the whole percentage (aria-valuemin 0, aria-valuemax 100) and aria-valuetext the text, if any.
     *
     * @code
     * auto done = std::make_shared<Nui::Observed<std::int64_t>>(0);
     * auto total = std::make_shared<Nui::Observed<std::int64_t>>(4096);
     * progressBar({.current = done, .maximum = total, .textMode = ProgressBarTextMode::Bytes});
     * @endcode
     *
     * @param options The options.
     * @return The bar.
     */
    Nui::ElementRenderer progressBar(ProgressBarOptions options);

    /**
     * @brief Settings of the imperative ProgressBar, the fields of the former host progress bar plus new ones.
     */
    struct ProgressBarSettings
    {
        /**
         * @brief CSS height, applied inline to the bar like before.
         */
        std::string height{"30px"};

        /**
         * @brief The lower bound.
         */
        std::int64_t minimum{0};

        /**
         * @brief The initial upper bound.
         */
        std::int64_t maximum{100};

        /**
         * @brief Shows current and maximum next to the percentage: Counts, or Bytes with byteMode. False shows
         *        Percent.
         */
        bool showMinimumAndMaximum{false};

        /**
         * @brief Formats current and maximum as byte sizes when showMinimumAndMaximum is set.
         */
        bool byteMode{false};

        /**
         * @brief How the fill is colored.
         */
        ProgressColorMode colorMode{ProgressColorMode::Hue};

        /**
         * @brief Runs the shine animation while the bar is not complete.
         */
        bool shine{true};

        /**
         * @brief The built-in texts.
         */
        ProgressBarTexts texts{};

        /**
         * @brief Extra attributes merged onto the bar root on every render.
         */
        std::vector<Nui::Attribute> attributes{};
    };

    /**
     * @brief A progress bar driven by setters, rendering the same markup as progressBar().
     *
     * Setters only change observed state and never sync; the caller runs
     * Nui::globalEventContext.executeActiveEventsImmediately() once when it is done.
     *
     * @code
     * ScriptNuiComponents::ProgressBar bar_{{
     *     .height = "11px",
     *     .maximum = 0,
     *     .showMinimumAndMaximum = true,
     *     .byteMode = true,
     * }};
     * // in the render tree:
     * bar_()
     * // on progress:
     * bar_.setMaximum(total);
     * bar_.setProgress(done);
     * Nui::globalEventContext.executeActiveEventsImmediately();
     * @endcode
     */
    class ProgressBar
    {
      public:
        using Settings = ProgressBarSettings;

        /**
         * @brief Creates the bar.
         *
         * @param settings The settings; the text mode follows from showMinimumAndMaximum and byteMode.
         */
        explicit ProgressBar(Settings settings = {});
        ~ProgressBar();
        ProgressBar(ProgressBar const&) = delete;
        ProgressBar(ProgressBar&&) noexcept;
        ProgressBar& operator=(ProgressBar const&) = delete;
        ProgressBar& operator=(ProgressBar&&) noexcept;

        /**
         * @brief Renders the bar. May be called repeatedly; every render follows the same state.
         *
         * @param extraStyleOptions CSS declarations appended to the inline style of the bar root.
         * @return The bar.
         */
        Nui::ElementRenderer operator()(std::string const& extraStyleOptions = "") const;

        /**
         * @brief Sets the current value. Does not sync.
         *
         * @param current The current value.
         */
        void setProgress(std::int64_t current);

        /**
         * @brief The current value.
         *
         * @return The value last set, initially Settings::minimum.
         */
        std::int64_t progress() const;

        /**
         * @brief Sets the upper bound. Does not sync.
         *
         * @param maximum The upper bound.
         */
        void setMaximum(std::int64_t maximum);

        /**
         * @brief The upper bound.
         *
         * @return The value last set, initially Settings::maximum.
         */
        std::int64_t maximum() const;

        /**
         * @brief Fills a bar whose maximum is 0 completely (100 %) from now on. Does not sync.
         *
         * @param zeroAsComplete Whether a maximum of 0 fills the bar.
         */
        void setZeroAsComplete(bool zeroAsComplete = true);

        /**
         * @brief Switches what the bar writes onto itself. Does not sync.
         *
         * @param textMode The new text mode.
         */
        void setTextMode(ProgressBarTextMode textMode);

        /**
         * @brief What the bar writes onto itself.
         *
         * @return The text mode.
         */
        ProgressBarTextMode textMode() const;

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };

} // namespace ScriptNuiComponents
