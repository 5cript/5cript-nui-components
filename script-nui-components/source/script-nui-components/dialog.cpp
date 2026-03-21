#include <script-nui-components/dialog.hpp>
#include <script-nui-components/button.hpp>

#include <emscripten/val.h>

#include <nui/frontend/api/event.hpp>
#include <nui/frontend/api/keyboard_event.hpp>

#include <fmt/format.h>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;

namespace ScriptNuiComponents
{
    struct Dialog::Implementation
    {
        std::string id;
        ElementRenderer body;

        Observed<std::string> headerText;
        Observed<Button> buttons;

        std::optional<std::variant<Dialog::Button, std::string>> initialFocus{std::nullopt};
        Observed<StyleVariant> styleVariant{StyleVariant::Regular};
        std::function<void(std::optional<Button> buttonPressed)> onClose;

        Observed<std::string> okButtonLabel{"Ok"};
        Observed<std::string> cancelButtonLabel{"Cancel"};
        Observed<std::string> yesButtonLabel{"Yes"};
        Observed<std::string> noButtonLabel{"No"};
        Observed<std::string> allButtonLabel{"All"};
        Observed<std::string> noneButtonLabel{"None"};

        std::weak_ptr<Dom::BasicElement> dialog{};
        bool mayCloseWithoutButton = false;
    };

    Dialog::Dialog(std::string id, ElementRenderer body)
        : impl_{std::make_unique<Implementation>(Implementation{
              .id = std::move(id),
              .body = std::move(body),
          })}
    {}

    Dialog::~Dialog() = default;
    Dialog::Dialog(Dialog&&) = default;
    Dialog& Dialog::operator=(Dialog&&) = default;

    void Dialog::open(OpenOptions const& options)
    {
        impl_->headerText = options.headerText;
        impl_->initialFocus = options.initialFocus;
        impl_->styleVariant = options.styleVariant;
        impl_->buttons = options.buttons;
        impl_->onClose = options.onClose;
        impl_->mayCloseWithoutButton = options.mayCloseWithoutButton;

        // If called outside an event handler, caller must flush manually.
        Nui::globalEventContext.executeActiveEventsImmediately();

        auto dialog = impl_->dialog.lock();
        if (dialog)
        {
            if (options.modal)
                dialog->val().call<void>("showModal");
            else
                dialog->val().call<void>("show");
            doFocus();
        }
    }

    void Dialog::doFocus()
    {
        if (!impl_->initialFocus)
            return;

        auto dialog = impl_->dialog.lock();
        if (!dialog)
            return;

        if (std::holds_alternative<Button>(*impl_->initialFocus))
        {
            auto button = std::get<Button>(*impl_->initialFocus);
            std::string buttonId = impl_->id + "_";
            switch (button)
            {
                case Button::Ok:
                    buttonId += "ok";
                    break;
                case Button::Cancel:
                    buttonId += "cancel";
                    break;
                case Button::Yes:
                    buttonId += "yes";
                    break;
                case Button::No:
                    buttonId += "no";
                    break;
                case Button::All:
                    buttonId += "all";
                    break;
                case Button::None:
                    buttonId += "none";
                    break;
                default:
                    WebApi::Console::error(
                        "Unknown button type for dialog initial focus:", static_cast<unsigned>(button)
                    );
                    return;
            }
            auto btn = Nui::val::global("document").call<Nui::val>("getElementById", buttonId);
            if (btn.isNull() || btn.isUndefined())
            {
                WebApi::Console::error("Could not find button with id", buttonId, "for dialog initial focus!");
                return;
            }
            Nui::WebApi::Console::log("Focusing button with id", buttonId, "for dialog initial focus.");
            btn.call<void>("focus");
        }
        else
        {
            auto elementId = std::get<std::string>(*impl_->initialFocus);
            auto elem = Nui::val::global("document").call<Nui::val>("getElementById", elementId);
            if (elem.isNull() || elem.isUndefined())
            {
                WebApi::Console::error("Could not find element with id", elementId, "for dialog initial focus!");
                return;
            }
            Nui::WebApi::Console::log("Focusing element with id", elementId, "for dialog initial focus.");
            elem.call<void>("focus");
        }
    }

    void Dialog::setButtonLabels(ButtonLabels const& labels)
    {
        impl_->okButtonLabel = labels.ok;
        impl_->cancelButtonLabel = labels.cancel;
        impl_->yesButtonLabel = labels.yes;
        impl_->noButtonLabel = labels.no;
        impl_->allButtonLabel = labels.all;
        impl_->noneButtonLabel = labels.none;
    }

    void Dialog::closeByButton(Button button)
    {
        if (impl_->onClose)
        {
            impl_->onClose(button);
        }
        auto dialog = impl_->dialog.lock();
        if (dialog)
        {
            dialog->val().call<void>("close");
        }
    }

    Nui::ElementRenderer Dialog::operator()()
    {
        using Nui::Elements::div;
        using ScriptNuiComponents::button;

        return dialog{
            id = impl_->id,
            class_ = "script-nui-dialog",
            reference = impl_->dialog,
            "cancel"_event =
                [this](Nui::val event)
            {
                if (!impl_->mayCloseWithoutButton)
                {
                    event.call<void>("preventDefault");
                    return;
                }

                // If the dialog was closed by calling dialog.close() in JS, we won't get a button value here, so we
                // treat it as if no button was pressed.
                if (impl_->onClose)
                    impl_->onClose(std::nullopt);
            }
        }(
            // Header
            div{class_ = observe(impl_->styleVariant)
                    .generate(
                        [this]()
                        {
                            return fmt::format("{} script-nui-dialog-header", toString(*impl_->styleVariant));
                        }
                    ),
                style = "flex: 0 0 auto;"}(impl_->headerText),

            // Body
            div{class_ = "script-nui-dialog-body", style = "flex: 1 1 auto;", tabIndex = 0}(impl_->body),

            // Footer
            div{
                class_ = "script-nui-dialog-footer",
                tabIndex = 0,
                "keydown"_event =
                    [this](Nui::WebApi::KeyboardEvent event)
                {
                    dialogButtonContainerKeydown(event);
                },
            }(fragment(
                  observe(impl_->buttons),
                  [this]() -> Nui::ElementRenderer
                  {
                      if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::Ok))
                      {
                          return button(
                              ButtonOptions{
                                  .text = impl_->okButtonLabel,
                                  .attributes =
                                      {id = impl_->id + "_ok",
                                          "click"_event =
                                              [this](Nui::val)
                                          {
                                              closeByButton(Button::Ok);
                                          },
                                          /*style = "grid-row: 1"*/},
                                  .styleVariant = StyleVariant::Regular
                              }
                          );
                      }
                      return Nui::nil();
                  }
              ),
                fragment(
                    observe(impl_->buttons),
                    [this]() -> Nui::ElementRenderer
                    {
                        if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::Yes))
                        {
                            return button(
                                {.text = impl_->yesButtonLabel,
                                    .attributes =
                                        {id = impl_->id + "_yes",
                                            "click"_event =
                                                [this](Nui::val)
                                            {
                                                closeByButton(Button::Yes);
                                            },
                                            /*style = "grid-row: 1"*/},
                                    .styleVariant = StyleVariant::Regular}
                            );
                        }
                        return Nui::nil();
                    }
                ),
                fragment(
                    observe(impl_->buttons),
                    [this]() -> Nui::ElementRenderer
                    {
                        if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::No))
                        {
                            return button(
                                {.text = impl_->noButtonLabel,
                                    .attributes =
                                        {id = impl_->id + "_no",
                                            "click"_event =
                                                [this](Nui::val)
                                            {
                                                closeByButton(Button::No);
                                            },
                                            /*style = "grid-row: 1"*/},
                                    .styleVariant = StyleVariant::Regular}
                            );
                        }
                        return Nui::nil();
                    }
                ),
                fragment(
                    observe(impl_->buttons),
                    [this]() -> Nui::ElementRenderer
                    {
                        if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::All))
                        {
                            return button(
                                {.text = impl_->allButtonLabel,
                                    .attributes =
                                        {id = impl_->id + "_all",
                                            "click"_event =
                                                [this](Nui::val)
                                            {
                                                closeByButton(Button::All);
                                            },
                                            /*style = "grid-row: 1"*/},
                                    .styleVariant = StyleVariant::Danger}
                            );
                        }
                        return Nui::nil();
                    }
                ),
                fragment(
                    observe(impl_->buttons),
                    [this]() -> Nui::ElementRenderer
                    {
                        if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::None))
                        {
                            return button(
                                {.text = impl_->noneButtonLabel,
                                    .attributes =
                                        {id = impl_->id + "_none",
                                            "click"_event =
                                                [this](Nui::val)
                                            {
                                                closeByButton(Button::None);
                                            },
                                            /*style = "grid-row: 1"*/},
                                    .styleVariant = StyleVariant::Regular}
                            );
                        }
                        return Nui::nil();
                    }
                ),
                fragment(
                    observe(impl_->buttons),
                    [this]() -> Nui::ElementRenderer
                    {
                        if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::Cancel))
                        {
                            return button(
                                {.text = impl_->cancelButtonLabel,
                                    .attributes =
                                        {
                                            id = impl_->id + "_cancel",
                                            "click"_event =
                                                [this](Nui::val)
                                            {
                                                closeByButton(Button::Cancel);
                                            },
                                            /*style = "grid-row: 1"*/
                                        },
                                    .styleVariant = StyleVariant::Regular}
                            );
                        }
                        return Nui::nil();
                    }
                ))
        );
    }

    void Dialog::dialogButtonContainerKeydown(Nui::WebApi::KeyboardEvent const& event)
    {
        using namespace std::string_literals;

        if (event.key() == "ArrowLeft" || event.key() == "ArrowUp" || event.key() == "ArrowRight" ||
            event.key() == "ArrowDown")
        {
            event.preventDefault();
            event.stopPropagation();
        }
        if (event.key() == "ArrowLeft" || event.key() == "ArrowUp")
        {
            auto button = event.currentTarget().call<Nui::val>("querySelector", "button:focus"s);

            if (button.isUndefined() || button.isNull())
                return;

            auto previous = button["previousElementSibling"];

            if (previous.isUndefined() || previous.isNull())
                previous = event.currentTarget().call<Nui::val>("querySelector", "button:last-child"s);

            if (previous.isUndefined() || previous.isNull())
                return;

            previous.call<void>("focus");
            return;
        }
        if (event.key() == "ArrowRight" || event.key() == "ArrowDown")
        {
            auto button = event.currentTarget().call<Nui::val>("querySelector", "button:focus"s);

            if (button.isUndefined() || button.isNull())
                return;

            auto next = button["nextElementSibling"];

            if (next.isUndefined() || next.isNull())
                next = event.currentTarget().call<Nui::val>("querySelector", "button"s);

            if (next.isUndefined() || next.isNull())
                return;

            next.call<void>("focus");
        }
    }
}