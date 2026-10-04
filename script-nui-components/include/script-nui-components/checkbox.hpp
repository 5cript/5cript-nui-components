#pragma once

#include <script-nui-components/state_transformers/text_node.hpp>

#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-checkbox)
#    include "../../../styles/checkbox.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace CheckboxDetail
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

    /**
     * @brief A themed checkbox with an optional label that is part of the click target.
     *
     * @param isChecked Checked state (plain value, Observed or combinator).
     * @param label Label text rendered right of the box; empty renders no label element.
     * @param attributes Extra attributes merged onto the checkbox root.
     * @param sizeFactor Scales the box; 1 is 16px.
     * @param onChange Invoked with the new state after a user toggle.
     * @param dontUpdateValue When true the bound isChecked value is left alone and only onChange
     *                        fires; for callers that own the state update themselves.
     */
    struct CheckboxOptions
    {
        Nui::StateTransformer<CheckboxDetail::IsCheckedReify> isChecked = false;
        Nui::StateTransformer<StateTransformers::TextNode> label = "";
        std::vector<Nui::Attribute> attributes = {};

        std::optional<double> sizeFactor = std::nullopt;
        std::function<void(bool, Nui::WebApi::MouseEvent const&)> onChange = {};
        bool dontUpdateValue = false;
    };

    Nui::ElementRenderer checkbox(CheckboxOptions options);

} // namespace ScriptNuiComponents
