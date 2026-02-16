#pragma once

#include "impl/attribute_traits.hpp"
#include "impl/merge_attributes.hpp"
#include "nui/frontend/api/console.hpp"

#include <nui/frontend/api/dom_rect_readonly.hpp>
#include <nui/frontend/dom/basic_element.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/svg/svg.hpp>
#include <nui/frontend/svg_elements.hpp>
#include <nui/frontend/svg_attributes.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <fmt/format.h>

#include <string>
#include <type_traits>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-select)
#    include "../../../styles/select.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace Detail
    {
        struct Positioning
        {
            double width;
            double top;
            double left;
        };
    }

    template <typename ActiveOptionType, typename OptionsValueType>
    struct SelectOptions
    {
        using ActiveType = Detail::AttributeTraits<ActiveOptionType>::Type;
        Detail::AttributeTraits<ActiveOptionType>::Actual activeOption = {};
        Detail::AttributeTraits<OptionsValueType>::Actual options = {};
        std::vector<Nui::Attribute> attributes = {};

        std::function<void(ActiveType const& newValue, Nui::WebApi::MouseEvent const&)> onChange = {};
        std::function<Nui::ElementRenderer(ActiveOptionType const&)> activeRenderer = [](ActiveOptionType const& actual)
        {
            return Nui::Elements::span{}(actual);
        };
        std::function<Nui::ElementRenderer(typename Detail::AttributeTraits<OptionsValueType>::Type::value_type const&)>
            elementRenderer = [](typename Detail::AttributeTraits<OptionsValueType>::Type::value_type const& each)
        {
            return Nui::Elements::span{}(each);
        };
        std::function<std::string()> makeId = []()
        {
            return Nui::val::global("crypto").call<std::string>("randomUUID");
        };
        bool dontUpdateValue = false;
    };

    template <typename ActiveOptionType, typename OptionsValueType>
    struct SelectState
    {
        SelectState(SelectOptions<ActiveOptionType, OptionsValueType>&& opts)
            : options{std::move(opts)}
        {}

        SelectOptions<ActiveOptionType, OptionsValueType> options;
        std::weak_ptr<Nui::Dom::BasicElement> buttonElement{};
        Nui::Observed<Detail::Positioning> positioning{{0.0, 0.0, 0.0}};
        std::string id = options.makeId();
    };

    template <typename ActiveOptionType, typename OptionsValueType>
    Nui::ElementRenderer select(SelectOptions<ActiveOptionType, OptionsValueType> options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using namespace std::string_literals;
        using Nui::Elements::span;
        using Nui::Elements::div;
        namespace svge = Nui::Elements::Svg;
        namespace svga = Nui::Attributes::Svg;

        auto state = std::make_shared<SelectState<ActiveOptionType, OptionsValueType>>(std::move(options));

        // clang-format off
            return Nui::Elements::button{
                mergeAttributes(
                    std::vector<Nui::Attribute>{
                        class_ = "script-nui-select",
                        onClick = [state](Nui::WebApi::MouseEvent mouseEvent) {
                            if (auto btn = state->buttonElement.lock(); btn)
                            {
                                auto rect = Nui::WebApi::DomRectReadOnly{btn->val().template call<Nui::val>("getBoundingClientRect")};
                                // const auto relativeLeft = btn->val()["offsetLeft"].template as<double>();
                                // const auto relativeTop = btn->val()["offsetTop"].template as<double>() + rect.height();
                                state->positioning = Detail::Positioning{
                                    .width = rect.width(),
                                    // .top = relativeTop,
                                    // .left = relativeLeft,
                                    .top = rect.top() + rect.height(),
                                    .left = rect.left()
                                };
                            }
                            mouseEvent.stopPropagation();
                        },
                        "popovertarget"_attr = fmt::format("select-options-{}", state->id),
                        reference = [state](std::weak_ptr<Nui::Dom::BasicElement> element) {
                            state->buttonElement = std::move(element);
                        },
                    },
                    std::move(state->options.attributes)
                )
            }(
                state->options.activeRenderer(state->options.activeOption),
                Nui::Elements::Svg::svg{
                    svga::viewBox = "0 0 24 24"s,
                }(
                    svge::path{
                        svga::d = "m6 9 6 6 6-6"s,
                    }()
                ),
                div{
                    "popover"_attr = "auto",
                    class_ = "script-nui-select-popover",
                    id = fmt::format("select-options-{}", state->id),
                    style = Nui::observe(state->positioning).generate([state]() {
                        return fmt::format("top: {}px; left: {}px; width: {}px;", state->positioning.value().top, state->positioning.value().left, state->positioning.value().width);
                    }),
                    onClick = [state, onChange = std::move(state->options.onChange)](Nui::WebApi::MouseEvent event) mutable {
                        event.val()["currentTarget"].call<void>("hidePopover");
                        event.stopPropagation();
                        auto target = event.target();
                        int max = 10; // paranoid
                        // while target is not div with data-index, keep looking up the parents:
                        while (!target.isNull() && !target.isUndefined() && !target.call<bool>("hasAttribute", "data-index"s) && !target.call<bool>("hasAttribute", "popover"s) && max-- > 0)
                        {
                            target = target["parentElement"];
                        }
                        if (target.isNull() || target.isUndefined() || target.call<bool>("hasAttribute", "popover"s))
                        {
                            Nui::WebApi::Console::error("Could not find option element for select click event!");
                            return; // dont know how.
                        }

                        const auto dataIndex = std::stol(target.call<std::string>("getAttribute", "data-index"s));
                        if (!state->options.dontUpdateValue)
                            Detail::AttributeTraits<ActiveOptionType>::assignValue(state->options.activeOption, Detail::AttributeTraits<OptionsValueType>::unwrap(state->options.options)[dataIndex]);
                        if (onChange)
                            onChange(Detail::AttributeTraits<OptionsValueType>::unwrap(state->options.options)[dataIndex], event);
                    },
                }(
                    Detail::AttributeTraits<OptionsValueType>::range(state->options.options),
                    [elementRenderer = std::move(state->options.elementRenderer)](long long index, auto const& item) -> Nui::ElementRenderer {
                        return div{
                            "data-index"_attr = static_cast<int>(index),
                        }(
                            elementRenderer(item)
                        );
                    }
                )
            );
        // clang-format on
    }
} // namespace ScriptNuiComponents