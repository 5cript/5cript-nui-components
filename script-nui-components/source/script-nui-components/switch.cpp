#include <script-nui-components/switch.hpp>

#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>

#include <nui/frontend/api/console.hpp>

#include <fmt/format.h>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer switch_(SwitchOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::span;
        using namespace std::string_literals;

        const auto styleVars = fmt::format(
            "{}{}{}{}",
            options.sizeFactor ? fmt::format("--size-factor: {};", *options.sizeFactor) : "",
            options.colorActive ? fmt::format("--color-active: {};", *options.colorActive) : "",
            options.colorInactive ? fmt::format("--color-inactive: {};", *options.colorInactive) : "",
            options.thumbColor ? fmt::format("--thumb-color: {};", *options.thumbColor) : ""
        );

        const auto [isChecked] = options.isChecked.reify();

        return Nui::Elements::button{Nui::mergeAttributes(
            {
                class_ = "script-nui-switch",
                isChecked,
                style = styleVars,
                onClick =
                    [isChecked = options.isChecked,
                        onChange = std::move(options.onChange),
                        dontUpdateValue = options.dontUpdateValue](Nui::WebApi::MouseEvent event) mutable
                {
                    event.stopPropagation();

                    const auto previousValue = isChecked.template value<bool>();

                    Nui::WebApi::Console::log(fmt::format("Switch toggled, new value: {}", !previousValue));
                    if (!dontUpdateValue)
                    {
                        isChecked.assign(!previousValue, Nui::ChangePolicy::Tracked);
                        event.target().call<void>("toggleAttribute", "data-is-checked"s);
                    }

                    if (onChange)
                        onChange(!previousValue, event);
                },
            },
            std::move(options.attributes)
        )}(span{}());
    }

} // namespace ScriptNuiComponents