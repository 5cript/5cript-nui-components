#pragma once

#include <nui/frontend/element_renderer.hpp>

#include <optional>
#include <string>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-spinner)
#    include "../../../styles/spinner.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /** @brief Optional overrides for a spinner instance.
     *  @param size       CSS length (e.g. "16px", "1.5em") applied to width + height.
     *  @param thickness  CSS length for the ring thickness.
     *  @param color      CSS color for the leading arc; trailing arcs are color-mixed.
     *
     *  Field order is size-thickness-color so typical callers setting size + thickness
     *  don't trip -Wmissing-designated-field-initializers by skipping a middle field.
     */
    struct SpinnerSettings
    {
        std::optional<std::string> size;
        std::optional<std::string> thickness;
        std::optional<std::string> color;
    };

    /** @brief A spinning-ring loading indicator.  Unset fields fall back to the
     *         public CSS custom properties (--script-nui-components-spinner-size,
     *         -color, -thickness) so global theming continues to work.
     */
    Nui::ElementRenderer spinner(SpinnerSettings settings = {});
} // namespace ScriptNuiComponents
