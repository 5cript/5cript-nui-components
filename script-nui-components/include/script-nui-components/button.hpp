#pragma once

#include <script-nui-components/state_transformers/text_node.hpp>
#include <script-nui-components/state_transformers/style_variant.hpp>
#include <script-nui-components/style_variant.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-button)
#    include "../../../styles/button.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /** @brief Configuration for an opt-in one-shot attention "shine" pulse.
     *  @param trigger Non-owning pointer to an Observed<int>; bump it to replay the animation.
     *  @param color   Optional CSS color override for the shine glow; empty = library default.
     */
    struct ShineOptions
    {
        Nui::Observed<int>* trigger = nullptr;
        std::string color = {};
    };

    struct ButtonOptions
    {
        Nui::StateTransformer<StateTransformers::TextNode> text = "";
        std::optional<Nui::ElementRenderer> icon = std::nullopt;
        std::vector<Nui::Attribute> attributes = {};
        Nui::StateTransformer<StateTransformers::StyleVariantTransformer> styleVariant = StyleVariant::Regular;
        std::optional<ShineOptions> shine = std::nullopt;
    };

    Nui::ElementRenderer button(ButtonOptions options);

} // namespace ScriptNuiComponents