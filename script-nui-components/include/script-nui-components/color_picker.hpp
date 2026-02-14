#pragma once

#include "impl/attribute_traits.hpp"
#include "impl/merge_attributes.hpp"
#include "nui/frontend/api/console.hpp"

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
    class ColorPicker
    {
      public:
        template <typename ValueType>
        struct Options
        {
            typename Detail::AttributeTraits<ValueType>::Actual value;
            std::string pickButtonText = "Pick";
            std::optional<Nui::ElementRenderer> pickButtonIcon = std::nullopt;
            StyleVariant buttonStyleVariant = StyleVariant::Regular;
        };

        template <typename ValueType>
        Nui::ElementRenderer operator()(Options<ValueType> options) const
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            auto labelElement = std::make_shared<std::weak_ptr<Nui::Dom::BasicElement>>();

            // clang-format off
            return div{
                class_ = "script-nui-color-picker",
            }(
                // Color Preview box:
                div{}(
                    div{
                        style = Detail::AttributeTraits<ValueType>::observe(options.value, [this](auto const& color) {
                            return fmt::format("background-color: {};", color);
                        })
                    }()
                ),
                TextInput{}(
                    TextInput::Options<ValueType>{
                        .value = options.value,
                        .attributes = {
                            pattern = "^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$",
                        },
                    }
                ),
                Button{}(
                    Button::Options{
                        .text = std::move(options.pickButtonText),
                        .icon = std::move(options.pickButtonIcon),
                        .styleVariant = options.buttonStyleVariant,
                        .attributes = {
                            Nui::Attributes::onClick = [labelElement](Nui::val event)
                            {
                                if (auto lbl = labelElement->lock(); lbl)
                                    lbl->val().call<void>("click", event);
                            }
                        }
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
                            Detail::AttributeTraits<ValueType>::assignValue(options.value, target["value"].template as<std::string>());
                    }
                }()
            );
            // clang-format on
        }
    };
} // namespace ScriptNuiComponents
