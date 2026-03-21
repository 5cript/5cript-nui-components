#pragma once

#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-input)
#    include "../../../styles/input.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    enum class ValueState
    {
        Valid,
        Invalid,
    };

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

        // When provided and set to Invalid, setCustomValidity is called on the
        // underlying <input> with validationMessage, triggering the CSS :invalid rule.
        // The Observed must outlive the rendered element.
        Nui::Observed<ValueState>* valueState = nullptr;

        // The message passed to setCustomValidity when valueState == Invalid.
        // Ignored when valueState is nullptr. The Observed must outlive the rendered element.
        Nui::Observed<std::string>* validationMessage = nullptr;
    };

    Nui::ElementRenderer textInput(TextInputOptions options);

} // namespace ScriptNuiComponents