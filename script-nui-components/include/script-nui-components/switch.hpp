#pragma once

#include "impl/attribute_traits.hpp"
#include "impl/merge_attributes.hpp"

#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>

#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <fmt/format.h>

#include <string>
#include <type_traits>
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
    template <typename CheckedType>
    struct SwitchOptions
    {
        typename Detail::AttributeTraits<CheckedType>::Actual isChecked = false;
        std::vector<Nui::Attribute> attributes = {};

        std::optional<double> sizeFactor = std::nullopt;
        std::optional<std::string> colorActive = std::nullopt;
        std::optional<std::string> colorInactive = std::nullopt;
        std::optional<std::string> thumbColor = std::nullopt;
        std::function<void(bool, Nui::WebApi::MouseEvent const&)> onChange = {};
        bool dontUpdateValue = false;
    };

    template <typename CheckedType = bool>
    Nui::ElementRenderer switch_(SwitchOptions<CheckedType> options)
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

        return Nui::Elements::button{mergeAttributes(
            {
                class_ = "script-nui-switch",
                "data-is-checked"_attr = options.isChecked,
                style = styleVars,
                onClick =
                    [isChecked = options.isChecked,
                        onChange = std::move(options.onChange),
                        dontUpdateValue = options.dontUpdateValue](Nui::WebApi::MouseEvent event) mutable
                {
                    event.stopPropagation();

                    Detail::AttributeTraits<CheckedType>::assignValue(
                        isChecked, !Detail::AttributeTraits<CheckedType>::getValue(isChecked)
                    );

                    // Not updated via the observed variable, so toggle the attribute manually.
                    if constexpr (std::is_same_v<std::decay_t<CheckedType>, bool>)
                    {
                        if (!dontUpdateValue)
                            event.target().call<void>("toggleAttribute", "data-is-checked"s);
                    }
                    else
                    {
                        (void)dontUpdateValue;
                    }

                    if (onChange)
                        onChange(Detail::AttributeTraits<CheckedType>::getValue(isChecked), event);
                },
            },
            std::move(options.attributes)
        )}(span{}());
    }
} // namespace ScriptNuiComponents
