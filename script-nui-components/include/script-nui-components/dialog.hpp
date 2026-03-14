#pragma once

#include <script-nui-components/style_variant.hpp>

#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/keyboard_event.hpp>

#include <memory>
#include <string>

namespace ScriptNuiComponents
{
    class Dialog
    {
      public:
        enum class Button : unsigned
        {
            Unknown = 0b0000'0000,
            Cancel = 0b0000'0001,
            Ok = 0b0000'0010,
            Yes = 0b0000'0100,
            No = 0b0000'1000,
            All = 0b0001'0000,
            None = 0b0010'0000
        };

        friend auto operator|(Button lhs, Button rhs)
        {
            return static_cast<Button>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
        }

        Dialog(std::string id, Nui::ElementRenderer body);

        ~Dialog();
        Dialog(Dialog const&) = delete;
        Dialog(Dialog&&);
        Dialog& operator=(Dialog const&) = delete;
        Dialog& operator=(Dialog&&);

        struct OpenOptions
        {
            StyleVariant styleVariant = StyleVariant::Regular;
            std::string headerText = "";
            Button buttons = Button::Ok;
            /// Focus on a button or an element by id:
            std::optional<std::variant<Button, std::string>> initialFocus = std::nullopt;
            /// Called when the dialog is closed.
            std::function<void(std::optional<Button> buttonPressed)> onClose = [](auto) {};
            bool modal{true};
            bool mayCloseWithoutButton{false};
        };

        struct ButtonLabels
        {
            std::string ok = "Ok";
            std::string cancel = "Cancel";
            std::string yes = "Yes";
            std::string no = "No";
            std::string all = "All";
            std::string none = "None";
        };
        void setButtonLabels(ButtonLabels const& labels);

        /// Open the dialog.
        void open(OpenOptions const& options);

        /// Close the dialog.
        void close();

        Nui::ElementRenderer operator()();

      private:
        void dialogButtonContainerKeydown(Nui::WebApi::KeyboardEvent const& event);
        void closeByButton(Button button);
        void doFocus();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}