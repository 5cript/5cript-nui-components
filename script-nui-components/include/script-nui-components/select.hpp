#pragma once

#include <script-nui-components/impl/attribute_traits.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>

#include <nui/frontend/api/dom_rect_readonly.hpp>
#include <nui/frontend/dom/basic_element.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/svg/svg.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/svg_elements.hpp>
#include <nui/frontend/svg_attributes.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/api/console.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <type_traits>
#include <optional>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-select)
#    include "../../../styles/select.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace SelectDetail
    {
        struct Positioning
        {
            double width;
            double top;
            double left;
            double maxHeight;
            bool flipUp; // true when opening upward (more space above than below)
        };
    }

    template <typename ActiveOptionType, typename OptionsValueType>
    struct SelectOptions
    {
        using ActiveType = Detail::AttributeTraits<ActiveOptionType>::Type;
        Detail::AttributeTraits<ActiveOptionType>::Actual activeOption = {};
        Detail::AttributeTraits<OptionsValueType>::Actual options = {};
        std::vector<Nui::Attribute> attributes = {};
        std::optional<std::string> popoverExtraClass = std::nullopt;

        std::function<void(ActiveType const& newValue, Nui::WebApi::MouseEvent const&)> onChange = {};
        std::function<Nui::ElementRenderer(typename Detail::AttributeTraits<ActiveOptionType>::Actual&)>
            activeRenderer = [](typename Detail::AttributeTraits<ActiveOptionType>::Actual& actual)
        {
            return Nui::Elements::span{}(Detail::AttributeTraits<ActiveOptionType>::asChild(actual));
        };
        std::function<Nui::ElementRenderer(typename Detail::AttributeTraits<OptionsValueType>::Type::value_type const&)>
            elementRenderer = [](typename Detail::AttributeTraits<OptionsValueType>::Type::value_type const& each)
        {
            if constexpr (Detail::AttributeTraits<OptionsValueType>::isMap)
            {
                return Nui::Elements::span{}(each.first);
            }
            else
                return Nui::Elements::span{}(each);
        };
        std::function<std::string()> makeId = []()
        {
            const auto crypto = Nui::val::global("crypto");
            if (crypto.isUndefined() || crypto.isNull() || !crypto.hasOwnProperty("randomUUID"))
            {
                // fallback if crypto.randomUUID is not available for some reason:
                Nui::WebApi::Console::warn(
                    "crypto.randomUUID is not available, falling back to less robust id generation for select "
                    "component. You should pass makeId to SelectOptions!"
                );
                static int counter = 0;
                return fmt::format("select-{}", counter++);
            }
            return crypto.call<std::string>("randomUUID");
        };
        std::function<bool()> onOpen = {};
        bool dontUpdateValue = false;

        // Upper bound on the popover height in pixels. The actual height will be
        // clamped to the available viewport space as well, so this is a maximum,
        // not a fixed value. Can also be overridden via the CSS variable
        // --script-nui-components-select-max-height. Set to 0 to disable the
        // option-based cap (viewport clamping still applies).
        double maxHeight = 300.0;
    };

    template <typename ActiveOptionType, typename OptionsValueType>
    struct SelectState
    {
        SelectState(SelectOptions<ActiveOptionType, OptionsValueType>&& opts)
            : options{std::move(opts)}
        {}

        SelectOptions<ActiveOptionType, OptionsValueType> options;
        std::weak_ptr<Nui::Dom::BasicElement> buttonElement{};
        Nui::Observed<SelectDetail::Positioning> positioning{{0.0, 0.0, 0.0, 300.0, false}};
        Nui::Observed<bool> loading{false};
        std::string id = options.makeId();
        std::unique_ptr<Nui::ListenRemover<typename Detail::AttributeTraits<OptionsValueType>::Listenable>>
            optionsListener;
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
                    "data-loading"_attr = state->loading,
                    onClick = [state](Nui::WebApi::MouseEvent mouseEvent) {
                        auto btn = state->buttonElement.lock();
                        if (!btn)
                        {
                            Nui::WebApi::Console::error("Could not get button element for select component!");
                            return;
                        }

                        auto rect = Nui::WebApi::DomRectReadOnly{btn->val().template call<Nui::val>("getBoundingClientRect")};
                        const double viewportHeight = Nui::val::global("window")["innerHeight"].template as<double>();
                        const double spaceBelow = viewportHeight - rect.top() - rect.height();
                        const double spaceAbove = rect.top();
                        const bool flipUp = spaceBelow < state->options.maxHeight && spaceAbove > spaceBelow;
                        const double availableSpace = flipUp ? spaceAbove : spaceBelow;
                        const double cssMax = state->options.maxHeight > 0.0 ? state->options.maxHeight : availableSpace;
                        const double computedMaxHeight = std::max(0.0, std::min(availableSpace, cssMax) - 8.0); // 8px breathing room
                        const double top = flipUp
                            ? rect.top()
                            : rect.top() + rect.height();
                        state->positioning = SelectDetail::Positioning{
                            .width = rect.width(),
                            .top = top,
                            .left = rect.left(),
                            .maxHeight = computedMaxHeight,
                            .flipUp = flipUp,
                        };
                        mouseEvent.stopPropagation();
                        if (state->options.onOpen)
                        {
                            const auto loading = state->options.onOpen();
                            if constexpr (Detail::AttributeTraits<OptionsValueType>::isObserved)
                            {
                                state->loading = loading;
                                if (loading)
                                {
                                    state->optionsListener = std::make_unique<Nui::ListenRemover<typename Detail::AttributeTraits<OptionsValueType>::Listenable>>(
                                        Detail::AttributeTraits<OptionsValueType>::smartListen(state->options.options, [state](auto&&...) {
                                            state->loading = false;
                                        }));
                                }
                            }
                        }
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
            span{class_ = "script-nui-select-spinner"}(),
            div{
                "popover"_attr = "auto",
                class_ = fmt::format("script-nui-select-popover {}", state->options.popoverExtraClass.value_or("")),
                id = fmt::format("select-options-{}", state->id),
                style = Nui::observe(state->positioning).generate([state]() {
                    const auto& p = state->positioning.value();
                    return fmt::format(
                        "top: {}px; left: {}px; width: {}px; max-height: {}px; overflow-y: auto;"
                        "transform-origin: {}; transform: {};",
                        p.top, p.left, p.width, p.maxHeight,
                        p.flipUp ? "bottom center" : "top center",
                        p.flipUp ? "translateY(-100%)" : "none"
                    );
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

                    if constexpr (Detail::AttributeTraits<OptionsValueType>::isMap)
                    {
                        auto key = target.call<std::string>("getAttribute", "data-key"s);
                        auto& optionsMap = Detail::AttributeTraits<OptionsValueType>::unwrap(state->options.options);
                        auto iter = optionsMap.find(key);
                        if (iter == optionsMap.end())
                        {
                            Nui::WebApi::Console::error("Could not find option for select with key: {}", key);
                            return;
                        }
                        if (!state->options.dontUpdateValue)
                            Detail::AttributeTraits<ActiveOptionType>::assignValue(state->options.activeOption, iter->second);
                        if (onChange)
                            onChange(iter->second, event);
                    }
                    else
                    {
                        const auto dataIndex = std::stol(target.call<std::string>("getAttribute", "data-index"s));
                        if (!state->options.dontUpdateValue)
                            Detail::AttributeTraits<ActiveOptionType>::assignValue(state->options.activeOption, Detail::AttributeTraits<OptionsValueType>::unwrap(state->options.options)[dataIndex]);
                        if (onChange)
                            onChange(Detail::AttributeTraits<OptionsValueType>::unwrap(state->options.options)[dataIndex], event);
                    }
                },
            }(
                Detail::AttributeTraits<OptionsValueType>::range(state->options.options),
                [elementRenderer = std::move(state->options.elementRenderer)](long long index, auto const& item) -> Nui::ElementRenderer {
                    return div{
                        "data-index"_attr = static_cast<int>(index),
                        "data-key"_attr = [&item]() -> std::optional<std::string> {
                            if constexpr (Detail::AttributeTraits<OptionsValueType>::isMap)
                            {
                                return std::make_optional<std::string>(item.first);
                            }
                            else
                            {
                                (void)item;
                                return std::nullopt;
                            }
                        }()
                    }(
                        elementRenderer(item)
                    );
                }
            )
        );
        // clang-format on
    }
} // namespace ScriptNuiComponents