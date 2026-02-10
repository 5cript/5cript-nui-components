#pragma once

#include "impl/attribute_traits.hpp"
#include "impl/merge_attributes.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <fmt/format.h>

#include <string>
#include <type_traits>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-input)
#    include "../../../styles/input.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    class TextInput
    {
      public:
        template <typename ValueType>
        struct Options
        {
            Detail::AttributeTraits<ValueType>::Actual value = {};
            std::vector<Nui::Attribute> attributes = {};

            std::function<void(std::string const& newValue, Nui::WebApi::Event const&)> onChange = {};
        };

        template <typename ValueType>
        Nui::ElementRenderer operator()(Options<ValueType> options) const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using namespace std::string_literals;

            // clang-format off
            return input{
                mergeAttributes(
                    std::vector<Nui::Attribute>{
                        class_ = "script-nui-text-input", 
                        type = "text",
                        "value"_prop = options.value,
                        onChange = [value = options.value, onChange = std::move(options.onChange)](Nui::WebApi::Event event) mutable {
                            auto newValue = event.target()["value"].as<std::string>();
                            Detail::AttributeTraits<ValueType>::assignValue(value, std::move(newValue));
                            if (onChange)
                                onChange(newValue, event);
                        },
                    },
                    std::move(options.attributes)
                )
            }();
            // clang-format on
        }
    };
} // namespace ScriptNuiComponents