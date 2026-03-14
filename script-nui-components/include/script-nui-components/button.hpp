#pragma once

#include "impl/merge_attributes.hpp"
#include "impl/attribute_traits.hpp"

#include <script-nui-components/style_variant.hpp>

#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/text.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/class.hpp>

#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <fmt/format.h>

#include <string>
#include <type_traits>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-button)
#    include "../../../styles/button.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    template <typename TextType>
    struct ButtonOptions
    {
        typename Detail::AttributeTraits<TextType>::Actual text{};
        std::optional<Nui::ElementRenderer> icon = std::nullopt;
        std::vector<Nui::Attribute> attributes = {};
        StyleVariant styleVariant = StyleVariant::Regular;
    };

    template <typename TextType>
    inline Nui::ElementRenderer button(ButtonOptions<TextType> options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;

        auto button = Nui::Elements::button{mergeAttributes(
            std::vector<Nui::Attribute>{
                class_ = "script-nui-button",
                "data-variant"_attr = toString(options.styleVariant),
            },
            std::move(options.attributes)
        )};

        if (options.icon)
            return std::move(button)(*options.icon, Nui::Elements::text{std::move(options.text)}());
        return std::move(button)(std::move(options.text));
    }

    inline Nui::ElementRenderer button(ButtonOptions<std::string> options)
    {
        return button<std::string>(std::move(options));
    }
} // namespace ScriptNuiComponents
