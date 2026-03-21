#pragma once

#include <script-nui-components/impl/merge_attributes.hpp>

#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>

#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/state_transformer.hpp>

#include <fmt/format.h>

#include <string>
#include <optional>
#include <string>
#include <vector>
#include <functional>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-switch)
#    include "../../../styles/switch.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace SwitchDetail
    {
        struct IsCheckedReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                using namespace Nui::Attributes::Literals;
                return "data-is-checked"_attr = statefulObject;
            }
        };
    }

    struct SwitchOptions
    {
        Nui::StateTransformer<SwitchDetail::IsCheckedReify> isChecked = false;
        std::vector<Nui::Attribute> attributes = {};

        std::optional<double> sizeFactor = std::nullopt;
        std::optional<std::string> colorActive = std::nullopt;
        std::optional<std::string> colorInactive = std::nullopt;
        std::optional<std::string> thumbColor = std::nullopt;
        std::function<void(bool, Nui::WebApi::MouseEvent const&)> onChange = {};
        bool dontUpdateValue = false;
    };

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

        return Nui::Elements::button{mergeAttributes(
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
