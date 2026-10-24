#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace ScriptNuiComponents
{
    /**
     * @brief How a progress indicator colors its fill.
     */
    enum class ProgressColorMode
    {
        /**
         * @brief Red while little is done, turning green on completion, see progressHueColor().
         */
        Hue,

        /**
         * @brief The theme color variable of the component.
         */
        Theme,
    };

    /**
     * @brief The value of the data-color-mode attribute for a color mode.
     *
     * @param colorMode The color mode.
     * @return "hue" or "theme".
     */
    inline std::string toString(ProgressColorMode colorMode)
    {
        switch (colorMode)
        {
            case ProgressColorMode::Hue:
                return "hue";
            case ProgressColorMode::Theme:
                return "theme";
        }
        return "hue";
    }

    /**
     * @brief The hue of a progress fill: 120 * fraction^1.8, at least 5, and 120 once complete.
     *
     * @param fraction Progress from 0 to 1. Values below 0 count as 0, values from 1 on as complete.
     * @return The hue in degrees, from 5 to 120.
     */
    inline double progressHue(double fraction)
    {
        if (fraction >= 1.0)
            return 120.0;
        const auto clamped = fraction > 0.0 ? fraction : 0.0;
        return std::max(5.0, 120.0 * std::pow(clamped, 1.8));
    }

    /**
     * @brief The fill color of a progress indicator in ProgressColorMode::Hue.
     *
     * @param fraction Progress from 0 to 1, see progressHue().
     * @return The CSS color "hsl(h, 40%, 45%)", the hue truncated to whole degrees.
     */
    inline std::string progressHueColor(double fraction)
    {
        return fmt::format("hsl({}, 40%, 45%)", static_cast<int>(progressHue(fraction)));
    }

} // namespace ScriptNuiComponents
