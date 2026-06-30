#pragma once

#include <script-nui-components/state_transformers/text_node.hpp>

#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/element_renderer.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-collapsible-section)
#    include "../../../styles/collapsible_section.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /**
     * @brief A titled, expandable section (native <details>/<summary>): a header with a chevron,
     *        a label and an optional count badge, over a collapsible body.
     *
     * @param title Header label (plain value, Observed or combinator).
     * @param badge Optional small count/label shown next to the title.
     * @param initiallyExpanded Whether the section starts open.
     * @param onToggle Optional callback fired on expand/collapse with the new open state.
     * @param attributes Extra attributes merged onto the <details> root.
     */
    struct CollapsibleSectionOptions
    {
        Nui::StateTransformer<StateTransformers::TextNode> title = "";
        std::optional<std::string> badge = std::nullopt;
        bool initiallyExpanded = true;
        std::function<void(bool expanded)> onToggle = {};
        std::vector<Nui::Attribute> attributes = {};
    };

    Nui::ElementRenderer collapsibleSection(CollapsibleSectionOptions options, Nui::ElementRenderer content);

} // namespace ScriptNuiComponents
