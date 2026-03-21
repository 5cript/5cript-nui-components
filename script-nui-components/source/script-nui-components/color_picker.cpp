#include <script-nui-components/color_picker.hpp>

#include <script-nui-components/text_input.hpp>
#include <script-nui-components/button.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/input.hpp>
#include <nui/frontend/elements/label.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/text.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/on_change.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/reference.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/frontend/attributes/type.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/api/mouse_event.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer colorPicker(ColorPickerOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        auto labelElement = std::make_shared<std::weak_ptr<Nui::Dom::BasicElement>>();

        auto [previewBoxStyle, textInputElement] = options.value.reify();

        // clang-format off
        return div{
            mergeAttributes(std::move(options.attributes), {
                class_ = "script-nui-color-picker",
            })
        }(
            // Color preview box:
            div{}(
                div{
                    std::move(previewBoxStyle)
                }()
            ),
            std::move(textInputElement),
            button(
                {
                    .text = std::move(options.pickButtonText),
                    .icon = std::move(options.pickButtonIcon),
                    .attributes = {
                        Nui::Attributes::onClick = [labelElement](Nui::val event)
                        {
                            if (auto lbl = labelElement->lock(); lbl)
                                lbl->val().call<void>("click", event);
                        }
                    },
                    .styleVariant = options.buttonStyleVariant,
                }
            ),
            input{
                type = "color",
                reference = [labelElement](std::weak_ptr<Nui::Dom::BasicElement> element) {
                    *labelElement = std::move(element);
                },
                style = "visibility: hidden",
                onChange =
                    [options](Nui::WebApi::Event event)
                {
                    auto target = event.target();
                    if (!target.isUndefined())
                    {
                        std::string value;
                        value = target["value"].template as<std::string>();
                        if (!options.dontUpdateValue)
                            options.value.assign(value, Nui::ChangePolicy::Tracked);
                        if (options.onChange)
                            options.onChange(value);
                    }
                }
            }()
        );
        // clang-format on
    }

} // namespace ScriptNuiComponents