#include <script-nui-components/spinner.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>

#include <fmt/format.h>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer spinner(SpinnerSettings settings)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        const auto styleVars = fmt::format(
            "{}{}{}",
            settings.size ? fmt::format("--script-nui-components-spinner-size: {};", *settings.size) : "",
            settings.color ? fmt::format("--script-nui-components-spinner-color: {};", *settings.color) : "",
            settings.thickness ? fmt::format("--script-nui-components-spinner-thickness: {};", *settings.thickness) : ""
        );

        if (styleVars.empty())
            return div{class_ = "script-nui-spinner"}();
        return div{class_ = "script-nui-spinner", style = styleVars}();
    }
} // namespace ScriptNuiComponents
