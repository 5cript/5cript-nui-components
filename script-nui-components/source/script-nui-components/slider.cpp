#include <script-nui-components/slider.hpp>

#include <nui/frontend/elements/input.hpp>

#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/type.hpp>
#include <nui/frontend/attributes/min.hpp>
#include <nui/frontend/attributes/max.hpp>
#include <nui/frontend/attributes/step.hpp>
#include <nui/frontend/attributes/on_input.hpp>
#include <nui/frontend/attributes/on_change.hpp>

#include <charconv>
#include <string>

namespace ScriptNuiComponents
{
    namespace
    {
        /**
         * @brief Read the current value of a range input from a DOM event. The DOM
         * exposes `value` as a string; parse it into an int with std::from_chars.
         */
        int readRangeValue(Nui::WebApi::Event const& event, int fallback) noexcept
        {
            const auto target = event.val()["target"];
            if (target.isUndefined() || target.isNull())
                return fallback;
            const auto raw = target["value"].as<std::string>();
            int parsed = fallback;
            std::from_chars(raw.data(), raw.data() + raw.size(), parsed);
            return parsed;
        }
    }

    Nui::ElementRenderer slider(SliderOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;

        const auto [valueAttribute] = options.value.reify();
        const bool dontUpdateValue = options.dontUpdateValue;
        auto boundValue = options.value;
        auto onInputCallback = std::move(options.onInput);
        auto onChangeCallback = std::move(options.onChange);

        return Nui::Elements::input{Nui::mergeAttributes(
            {
                class_ = "script-nui-slider",
                type = std::string{"range"},
                min = std::to_string(options.min),
                max = std::to_string(options.max),
                step = std::to_string(options.step),
                valueAttribute,
                onInput =
                    [boundValue,
                     dontUpdateValue,
                     onInputCallback = std::move(onInputCallback)](Nui::WebApi::Event event) mutable
                {
                    const int current = readRangeValue(event, boundValue.template value<int>());
                    if (!dontUpdateValue)
                        boundValue.assign(current, Nui::ChangePolicy::Tracked);
                    if (onInputCallback)
                        onInputCallback(current);
                },
                onChange =
                    [boundValue,
                     onChangeCallback = std::move(onChangeCallback)](Nui::WebApi::Event event) mutable
                {
                    if (!onChangeCallback)
                        return;
                    const int current = readRangeValue(event, boundValue.template value<int>());
                    onChangeCallback(current);
                },
            },
            std::move(options.attributes)
        )}();
    }
}
