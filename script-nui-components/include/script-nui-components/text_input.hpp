#pragma once

#include <script-nui-components/impl/merge_attributes.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/state_transformer.hpp>

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
    namespace TextInputDetail
    {
        struct ValueReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                using namespace Nui::Attributes::Literals;
                return "value"_prop = statefulObject;
            }
        };
    }

    struct TextInputOptions
    {
        Nui::StateTransformer<TextInputDetail::ValueReify> value;
        std::vector<Nui::Attribute> attributes = {};

        std::function<void(std::string const& newValue, Nui::WebApi::Event const&)> onChange = {};
        bool dontUpdateValue = false;
    };

    Nui::ElementRenderer textInput(TextInputOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using namespace std::string_literals;

        const auto [valueProp] = options.value.reify();

        // clang-format off
        return input{
            mergeAttributes(
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