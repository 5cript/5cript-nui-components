#pragma once

#include <script-nui-components/state_transformers/text_node.hpp>
#include <script-nui-components/style_variant.hpp>
#include <script-nui-components/text_input.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/attributes/pattern.hpp>
#include <nui/frontend/attributes/style.hpp>

#include <fmt/format.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-button)
#    include "../../../styles/color_picker.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace ColorPickerDetail
    {
        struct BackgroundColorReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, std::string const& statefulObject)
            {
                return Nui::Attributes::style = fmt::format("background-color: {};", statefulObject);
            }

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                return Nui::Attributes::style = observe(statefulObject)
                                                    .generate(
                                                        [](auto const& color)
                                                        {
                                                            return fmt::format("background-color: {};", color);
                                                        }
                                                    );
            }

            template <typename Generator, typename... ObservedT>
            static type reify(
                Nui::StateTransformerBase const&,
                Nui::ObservedValueCombinatorWithGenerator<Generator, ObservedT...>& statefulObject
            )
            {
                statefulObject.generate(
                    [orig = std::move(statefulObject).generator()](auto&&... values)
                    {
                        return fmt::format("background-color: {};", orig(std::forward<decltype(values)>(values)...));
                    }
                );
                return Nui::Attributes::style = statefulObject;
            }
        };

        struct TextInputReify
        {
            using type = Nui::ElementRenderer;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                return textInput({
                    .value = statefulObject,
                    .attributes = {
                        Nui::Attributes::pattern = "^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$",
                    },
                });
            }
        };
    }

    struct ColorPickerOptions
    {
        Nui::StateTransformer<ColorPickerDetail::BackgroundColorReify, ColorPickerDetail::TextInputReify> value;
        std::vector<Nui::Attribute> attributes = {};
        Nui::StateTransformer<StateTransformers::TextNode> pickButtonText = "Pick";
        std::optional<Nui::ElementRenderer> pickButtonIcon = std::nullopt;
        StyleVariant buttonStyleVariant = StyleVariant::Regular;
        std::function<void(std::string const& newColor)> onChange = {};
        bool dontUpdateValue = false;
    };

    Nui::ElementRenderer colorPicker(ColorPickerOptions options);

} // namespace ScriptNuiComponents