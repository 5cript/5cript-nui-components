#pragma once

#include <script-nui-components/style_variant.hpp>

#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>

namespace ScriptNuiComponents::StateTransformers
{
    struct StyleVariantTransformer
    {
        using type = Nui::Attribute;

        static type reify(Nui::StateTransformerBase const&, StyleVariant statefulObject)
        {
            using namespace Nui::Attributes::Literals;
            return "data-variant"_attr = toString(statefulObject);
        }

        static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
        {
            using namespace Nui::Attributes::Literals;
            return "data-variant"_attr = observe(statefulObject)
                                             .generate(
                                                 [](auto const& color)
                                                 {
                                                     return toString(color);
                                                 }
                                             );
        }

        template <typename Generator, typename... ObservedT>
        static type reify(
            Nui::StateTransformerBase const&,
            Nui::ObservedValueCombinatorWithGenerator<Generator, ObservedT...>& statefulObject
        )
        {
            using namespace Nui::Attributes::Literals;
            return "data-variant"_attr = statefulObject.regenerate(
                       [](StyleVariant variant)
                       {
                           return toString(variant);
                       }
                   );
            ;
        }
    };
}