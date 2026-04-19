#pragma once

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/api/event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-slider)
#    include "../../../styles/slider.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace SliderDetail
    {
        /**
         * @brief Binds the current numeric value to the underlying <input type="range">
         * through its `value` DOM property. Accepts plain values or Observed<int>.
         */
        struct ValueReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                using namespace Nui::Attributes::Literals;
                return "value"_prop = statefulObject;
            }
        };
    }

    /**
     * @brief Caller-facing options for the slider component.
     *
     * The slider wraps a native <input type="range">, so the usual `min`/`max`/`step`
     * semantics apply. The current value is bound via a StateTransformer so either a
     * plain integer or an Observed<int> can be passed.
     */
    struct SliderOptions
    {
        /** @brief The slider's current value (plain or observed). */
        Nui::StateTransformer<SliderDetail::ValueReify> value = 0;

        /** @brief Lower bound. */
        int min = 0;

        /** @brief Upper bound. */
        int max = 100;

        /** @brief Step size between discrete values. */
        int step = 1;

        /** @brief Extra attributes merged onto the <input> element. */
        std::vector<Nui::Attribute> attributes = {};

        /** @brief Fires on every input (drag). Receives the new integer value. */
        std::function<void(int)> onInput = {};

        /** @brief Fires when the user releases the slider. Receives the final value. */
        std::function<void(int)> onChange = {};

        /**
         * @brief When false (default) the component assigns the new value back to the
         * bound state on every input. Set to true if the caller wants to drive the
         * state externally (e.g. clamp, validate) via @p onInput.
         */
        bool dontUpdateValue = false;
    };

    /**
     * @brief Render a slider bound to the given options.
     *
     * The rendered element is a single native <input type="range"> carrying the
     * `script-nui-slider` CSS class. Pair with a neighbouring element displaying
     * the current value if needed.
     */
    Nui::ElementRenderer slider(SliderOptions options);
}
