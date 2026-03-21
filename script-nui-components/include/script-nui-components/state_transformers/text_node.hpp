#pragma once

#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/elements/text.hpp>

namespace ScriptNuiComponents::StateTransformers
{
    struct TextNode
    {
        using type = Nui::ElementRenderer;

        static Nui::ElementRenderer reify(Nui::StateTransformerBase const&, auto& statefulObject)
        {
            return Nui::Elements::text{statefulObject}();
        }
    };
}