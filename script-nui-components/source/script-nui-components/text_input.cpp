#include <script-nui-components/text_input.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/elements/input.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/on_change.hpp>
#include <nui/frontend/attributes/type.hpp>
#include <nui/frontend/attributes/reference.hpp>
#include <nui/frontend/dom/basic_element.hpp>
#include <nui/frontend/dom/reference.hpp>

#include <memory>

namespace ScriptNuiComponents
{
    namespace TextInputDetail
    {
        struct ValidationState
        {
            std::weak_ptr<Nui::Dom::BasicElement> element{};

            // Kept alive for the lifetime of the rendered element.
            Nui::ListenRemover<Nui::Observed<ValueState>> valueStateListener{};
            Nui::ListenRemover<Nui::Observed<std::string>> validationMessageListener{};
        };
    }

    Nui::ElementRenderer textInput(TextInputOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using namespace std::string_literals;

        const auto [valueProp] = options.value.reify();

        std::vector<Nui::Attribute> fixedAttributes{
            class_ = "script-nui-text-input",
            type = "text",
            valueProp,
            onChange = [theValue = options.value,
                           onChange = std::move(options.onChange),
                           dontUpdateValue = options.dontUpdateValue](Nui::WebApi::Event event) mutable
            {
                const auto newValue = event.target()["value"].as<std::string>();
                if (!dontUpdateValue)
                    theValue.assign(newValue, Nui::ChangePolicy::Tracked);
                if (onChange)
                    onChange(newValue, event);
            },
        };

        if (options.valueState != nullptr)
        {
            auto validState = std::make_shared<TextInputDetail::ValidationState>();

            Nui::Observed<ValueState>* valueState = options.valueState;

            // Helper that reads the current state from the Observed pointers and
            // calls setCustomValidity on the element.
            auto applyValidity = [validState, valueState, validationMessage = options.validationMessage]()
            {
                if (auto el = validState->element.lock(); el)
                {
                    const bool negative = valueState->value() == ValueState::Invalid;
                    const std::string msg = (negative && validationMessage) ? validationMessage->value() : ""s;
                    el->val().call<void>("setCustomValidity", msg);
                }
            };

            // Apply validity immediately when the element is first attached to the DOM.
            fixedAttributes.push_back(
                reference =
                    [validState, applyValidity](std::weak_ptr<Nui::Dom::BasicElement> el) mutable
                {
                    validState->element = std::move(el);
                    applyValidity();
                }
            );

            // Re-apply whenever valueState or validationMessage changes.
            // ListenRemovers are stored in validState so they are destroyed with it.
            validState->valueStateListener = Nui::smartListen(
                *valueState,
                [applyValidity](ValueState)
                {
                    applyValidity();
                }
            );

            validState->validationMessageListener = Nui::smartListen(
                *options.validationMessage,
                [applyValidity](std::string)
                {
                    applyValidity();
                }
            );
        }

        // clang-format off
        return input{
            Nui::mergeAttributes(
                std::move(fixedAttributes),
                std::move(options.attributes)
            )
        }();
        // clang-format on
    }

} // namespace ScriptNuiComponents