#include <script-nui-components/text_input.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/elements/input.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/on_change.hpp>
#include <nui/frontend/attributes/type.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer textInput(TextInputOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using namespace std::string_literals;

        const auto [valueProp] = options.value.reify();

        // clang-format off
        return input{
            Nui::mergeAttributes(
                std::vector<Nui::Attribute>{
                    class_ = "script-nui-text-input",
                    type = "text",
                    valueProp,
                    onChange = [value = options.value, onChange = std::move(options.onChange), dontUpdateValue = options.dontUpdateValue](Nui::WebApi::Event event) mutable {
                        const auto newValue = event.target()["value"].as<std::string>();
                        if (!dontUpdateValue)
                            value.assign(newValue, Nui::ChangePolicy::Tracked);

                        if (onChange)
                            onChange(newValue, event);
                    },
                },
                std::move(options.attributes)
            )
        }();
        // clang-format on
    }

} // namespace ScriptNuiComponents