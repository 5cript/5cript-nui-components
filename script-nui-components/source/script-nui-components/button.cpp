#include <script-nui-components/button.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>

#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/text.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/class.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer button(ButtonOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;

        auto btn = Nui::Elements::button{mergeAttributes(
            std::vector<Nui::Attribute>{
                class_ = "script-nui-button",
                "data-variant"_attr = toString(options.styleVariant),
            },
            std::move(options.attributes)
        )};

        auto [text] = options.text.reify();

        if (options.icon)
            return std::move(btn)(*options.icon, std::move(text));
        return std::move(btn)(std::move(text));
    }

} // namespace ScriptNuiComponents