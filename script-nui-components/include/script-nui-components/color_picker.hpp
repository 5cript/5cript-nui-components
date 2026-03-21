#pragma once

#include <script-nui-components/impl/merge_attributes.hpp>
#include <script-nui-components/state_transformers/text_node.hpp>

#include <script-nui-components/style_variant.hpp>
#include <script-nui-components/text_input.hpp>
#include <script-nui-components/color_picker.hpp>
#include <script-nui-components/button.hpp>

#include <nui/frontend/elements/label.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/text.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/class.hpp>

#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <fmt/format.h>

#include <string>
#include <type_traits>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-button)
#    include "../../../styles/button.css"
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

    Nui::ElementRenderer colorPicker(ColorPickerOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        auto labelElement = std::make_shared<std::weak_ptr<Nui::Dom::BasicElement>>();

        auto [previewBoxStyle, textInputElement] = options.value.reify();

        // clang-format off
        return div{
            mergeAttributes(std::move(options.attributes), {
                class_ = "script-nui-color-picker",
            })
        }(
            // Color Preview box:
            div{}(
                div{
                    std::move(previewBoxStyle)
                }()
            ),
            std::move(textInputElement),
            button(
                {
                    .text = std::move(options.pickButtonText),
                    .icon = std::move(options.pickButtonIcon),
                    .attributes = {
                        Nui::Attributes::onClick = [labelElement](Nui::val event)
                        {
                            if (auto lbl = labelElement->lock(); lbl)
                                lbl->val().call<void>("click", event);
                        }
                    },
                    .styleVariant = options.buttonStyleVariant,
                }
            ),
            input{
                type = "color",
                reference = [labelElement](std::weak_ptr<Nui::Dom::BasicElement> element) {
                    *labelElement = std::move(element);
                },
                style = "visibility: hidden",
                onChange =
                    [options](Nui::WebApi::Event event)
                {
                    auto target = event.target();
                    if (!target.isUndefined())
                    {
                        std::string value;
                        value = target["value"].template as<std::string>();
                        if (!options.dontUpdateValue)
                            options.value.assign(value, Nui::ChangePolicy::Tracked);
                        if (options.onChange)
                            options.onChange(value);
                    }
                }
            }()
        );
        // clang-format on
    }
} // namespace ScriptNuiComponents
