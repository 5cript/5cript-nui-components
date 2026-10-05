#pragma once

#include <script-nui-components/state_transformers/text_node.hpp>

#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/element_renderer.hpp>

#include <functional>
#include <optional>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-pill)
#    include "../../../styles/pill.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /**
     * @brief A small rounded chip used for tags, filters and status labels.
     *
     * @param text Label text (plain value, Observed or combinator).
     * @param icon Optional leading icon renderer.
     * @param selected Renders the pill in its active/selected style (data-selected).
     * @param onClick Optional click handler; when set the pill behaves like a toggle button.
     * @param onRemove Optional handler; when set a trailing remove (x) affordance is rendered.
     * @param attributes Extra attributes merged onto the pill root.
     */
    struct PillOptions
    {
        Nui::StateTransformer<StateTransformers::TextNode> text = "";
        std::optional<Nui::ElementRenderer> icon = std::nullopt;
        bool selected = false;
        std::function<void()> onClick = {};
        std::function<void()> onRemove = {};
        std::vector<Nui::Attribute> attributes = {};
    };

    Nui::ElementRenderer pill(PillOptions options);

} // namespace ScriptNuiComponents
