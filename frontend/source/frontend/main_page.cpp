#include <frontend/main_page.hpp>

#include <script-nui-components/switch.hpp>
#include <script-nui-components/text_input.hpp>
#include <script-nui-components/select.hpp>
#include <script-nui-components/button.hpp>
#include <script-nui-components/color_picker.hpp>
#include <script-nui-components/resizeable_table.hpp>
#include <script-nui-components/message_strip.hpp>
#include <script-nui-components/pill.hpp>
#include <script-nui-components/pill_list.hpp>
#include <script-nui-components/tag_box.hpp>
#include <script-nui-components/collapsible_section.hpp>
#include <script-nui-components/toast.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

using namespace std::string_literals;
using namespace ScriptNuiComponents;

MainPage::MainPage()
    : table_{
          ResizableTable::HeaderRow{
              ResizableTable::HeaderTableCell{"Header 1"s},
              ResizableTable::HeaderTableCell{"Header 2"s},
              ResizableTable::HeaderTableCell{
                  .content = "",
                  .initialWidth = 40,
                  .resizeable = false,
              }
          },
          ResizableTable::FooterFeature{{"Footer 1"s}, {"Footer 2"s}, {""s}},
          ResizableTable::AddFeature{
              .onAdd =
                  [](auto& table)
              {
                  const auto rowCount = table.rowCount();
                  table.addRow({
                      fmt::format("Row {}, Col 1", rowCount),
                      fmt::format("Row {}, Col 2", rowCount),
                      [](std::unique_ptr<ResizableTable::ISelfController> controller) -> Nui::ElementRenderer
                      {
                          std::shared_ptr<ResizableTable::ISelfController> sharedController = std::move(controller);
                          return button(
                              {.icon =
                                      Nui::Elements::Svg::svg{
                                          Nui::Attributes::Svg::viewBox = "0 0 512 512",
                                      }(Nui::Elements::Svg::path{
                                          Nui::Attributes::Svg::d =
                                              "M454 109q11 0 18.5 7t7.5 18-7.5 18.5T454 160h-19v294q0 24-17 41t-41 "
                                              "17H135q-24 0-41-17t-17-41V160H58q-11 0-18.5-7.5T32 134t7.5-18 "
                                              "18.5-7h70V58q0-24 17-41t41-17h140q24 0 41 17t17 41v51h70zm-275 "
                                              "0h154V58q0-7-7-7H186q-7 0-7 7v51zm205 51H128v294q0 7 7 7h242q7 0 "
                                              "7-7V160zm-186 64q11 0 18.5 7.5T224 250v140q0 11-7.5 18.5T198 "
                                              "416t-18-7.5-7-18.5V250q0-11 7-18.5t18-7.5zm116 0q11 0 18 7.5t7 "
                                              "18.5v140q0 "
                                              "11-7 18.5t-18 7.5-18.5-7.5T288 390V250q0-11 7.5-18.5T314 224z",
                                      }()),
                                  .attributes = {
                                      Nui::Attributes::onClick = [controller = std::move(sharedController)](auto const&)
                                      {
                                          Nui::WebApi::Console::log(
                                              "Button in row "s + std::to_string(controller->row()) + " clicked!"
                                          );
                                          controller->remove();
                                      }
                                  }}
                          );
                      },
                  });
              },
              .addNewEntryText = "Add new row",
          }
      }
    , dialog_{
          "MyDialog",
          Nui::Elements::div{}(
              Nui::Elements::label{}("Text Input:"),
              Nui::Elements::input{
                  Nui::Attributes::id = "text-input-1"s,
                  Nui::Attributes::type = "text"s,
                  Nui::Attributes::value = textInputValue_,
              }()
          )
      }
    , tagBox_{ScriptNuiComponents::TagBox::Options{
          .initialTags = {"alpha", "beta"},
          .placeholder = "add tag...",
          .onChange =
              [](std::vector<std::string> const& tags)
          {
              Nui::WebApi::Console::log("Tags changed, count: ", static_cast<int>(tags.size()));
          },
      }}
    , toast_{ScriptNuiComponents::ToastHostOptions{
          .style = ScriptNuiComponents::ToastStyle::Box,
          .position = ScriptNuiComponents::ToastPosition::BottomRight,
      }}
{
    rebuildFilterPills();

    tabs_.onSelect(
        [](std::size_t id)
        {
            Nui::WebApi::Console::log("Selected tab with id: ", id);
            return true;
        }
    );

    tabs_.onClose(
        [](std::size_t id)
        {
            Nui::WebApi::Console::log("Closed tab with id: ", id);
            return true;
        }
    );

    tabs_.onReorder(
        [](std::size_t from, std::size_t to)
        {
            Nui::WebApi::Console::log("Moved tab from index ", from, " to index ", to);
        }
    );

    popupMenu_.setItems({
        PopupMenu::item(
            "Action 1",
            std::string{},
            []()
            {
                Nui::WebApi::Console::log("Action 1 triggered");
            }
        ),
        PopupMenu::item(
            "Action 2 with shortcut",
            std::string{},
            []()
            {
                Nui::WebApi::Console::log("Action 2 triggered");
            },
            false,
            "Ctrl+S"
        ),
        PopupMenu::separator(),
        PopupMenu::sectionHeader("Section 1"),
        PopupMenu::item(
            "Disabled Action",
            std::string{},
            []()
            {
                Nui::WebApi::Console::log("This should not trigger");
            },
            true
        ),
        PopupMenu::item(
            "Action 3",
            std::string{},
            []()
            {
                Nui::WebApi::Console::log("Action 3 triggered");
            }
        ),
    });
}

// Renders a titled section with an underlined heading
Nui::ElementRenderer MainPage::section(std::string const& title, Nui::ElementRenderer content)
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    return div{
        style = "margin-bottom: 24px;"
    }(div{style = "font-size: 1.1em; font-weight: 600; border-bottom: 1px solid currentColor; padding-bottom: 4px; "
                  "margin-bottom: 12px;"}(text{title}()),
        std::move(content));
}

Nui::ElementRenderer MainPage::render()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using Nui::Elements::span;
    using ScriptNuiComponents::button;

    // clang-format off
    return body{}(
        dialog_(),
        popupMenu_(),
        toast_(),

        div{class_ = "top-bar"}(
            div{
                style = "display: flex; align-items: center; gap: 8px;",
            }(
                ScriptNuiComponents::switch_({
                    .isChecked = true,
                    .onChange =
                        [](bool isChecked, Nui::WebApi::MouseEvent const&)
                    {
                        if (isChecked)
                            Nui::val::global("document")["documentElement"].call<void>("setAttribute", "data-theme"s, "dark"s);
                        else
                            Nui::val::global("document")["documentElement"].call<void>("setAttribute", "data-theme"s, "light"s);
                    },
                }),
                span{}("Dark/Light Mode")
            )
        ),

        div{class_ = "content"}(
            div{}(
                section("Switch", switch_()),
                section("Text Input", textInput()),
                section("Select", select()),
                section("Icon Button", iconButton()),
                section("Color Picker", colorPicker()),
                section("Message Strips",
                    div{}(
                        messageStrip({.text = "Danger",      .styleVariant = StyleVariant::Danger}),
                        messageStrip({.text = "Warning",     .styleVariant = StyleVariant::Warning}),
                        messageStrip({.text = "Primary",     .styleVariant = StyleVariant::Primary}),
                        messageStrip({.text = "Success",     .styleVariant = StyleVariant::Success}),
                        messageStrip({.text = "Regular",     .styleVariant = StyleVariant::Regular}),
                        messageStrip({.text = "Transparent", .styleVariant = StyleVariant::Transparent})
                    )
                ),
                section("Pills", pills()),
                section("Pill List (reactive: add / toggle / remove)", pillListSection()),
                section("Tag Box", tagBoxSection()),
                section("Collapsible Sections", collapsibleSections()),
                section("Toasts (box vs. strip)", toastSection()),
                section("Buttons",
                    div{
                        style = "display: flex; gap: 5px;"
                    }(
                        button({
                            .text = observe(buttonTextSwitcher_).generate(std::function{[this]() {
                                return fmt::format("{}_suffix", buttonTextSwitcher_.value());
                            }}),
                            .attributes = {
                                onClick = [this]() {
                                    buttonTextSwitcher_ = (buttonTextSwitcher_.value() == "A") ? "B"s : "A"s;
                                }
                            },
                            .styleVariant = StyleVariant::Primary
                        }),
                        button({
                            .text = "Disabled Button",
                            .attributes = {
                                onClick = []() {
                                    Nui::WebApi::Console::log("This should not trigger");
                                },
                                disabled = true,
                            },
                            .styleVariant = StyleVariant::Primary
                        }),
                        button({
                            .text = "Regular",
                            .styleVariant = StyleVariant::Regular
                        }),
                        button({
                            .text = "Warning",
                            .styleVariant = StyleVariant::Warning
                        }),
                        button({
                            .text = "Danger",
                            .styleVariant = StyleVariant::Danger
                        }),
                        button({
                            .text = "Success",
                            .styleVariant = StyleVariant::Success
                        }),
                        button({
                            .text = "Transparent",
                            .styleVariant = StyleVariant::Transparent
                        })
                    )
                ),
                section("Popup Menu",
                    div{}(
                        button({
                            .text = "Open Popup Here",
                            .attributes = {
                                id      = "my-menu-btn-2"s,
                                onClick = [this](Nui::WebApi::MouseEvent) {
                                    popupMenu_.openNextTo("my-menu-btn-2");
                                }
                            },
                        })
                    )
                ),
                section("Dialog",
                    button({
                        .text = "Open Dialog",
                        .attributes = {
                            onClick = [this](auto const&) {
                                dialog_.open(Dialog::OpenOptions{
                                    .styleVariant = StyleVariant::Primary,
                                    .headerText = "Dialog Header",
                                    .buttons = Dialog::Button::Ok | Dialog::Button::Cancel | Dialog::Button::Yes | Dialog::Button::No,
                                    .initialFocus = Dialog::Button::Cancel,
                                    .onClose = [](std::optional<Dialog::Button> buttonPressed)
                                    {
                                        if (buttonPressed)
                                            Nui::WebApi::Console::log("Dialog closed with button: "s + std::to_string(static_cast<unsigned>(*buttonPressed)));
                                        else
                                            Nui::WebApi::Console::log("Dialog closed without pressing a button");
                                    },
                                    .modal = true,
                                });
                            }
                        },
                    })
                ),
                section("Tabs",
                    div{}(
                        button({
                            .text = "Add Tab",
                            .attributes = {
                                onClick = [this](auto const&) {
                                    static int tabCount = 0;
                                    tabs_.add("Tab "s + std::to_string(tabCount++), true);
                                }
                            },
                        }),
                        tabs_()
                    )
                ),
                section("Table",
                    table_({style = "max-height: 400px"})
                )
            )
        )
    );
    // clang-format on
}

Nui::ElementRenderer MainPage::switch_()
{
    return ScriptNuiComponents::switch_({
        .isChecked = isChecked_,
    });
}

Nui::ElementRenderer MainPage::textInput()
{
    return ScriptNuiComponents::textInput({
        .value = textInputValue_,
    });
}

Nui::ElementRenderer MainPage::select()
{
    return ScriptNuiComponents::select(
        SelectOptions<decltype(selectValue_), decltype(selectOptions_)>{
            .activeOption = selectValue_,
            .options = selectOptions_,
            .attributes =
                {
                    Nui::Attributes::disabled = false,
                    Nui::Attributes::style = "min-width: 200px",
                },
            .onChange = [](std::string const& newValue, auto const&)
            {
                Nui::WebApi::Console::log("Selected option: "s + newValue);
            }
        }
    );
}

Nui::ElementRenderer MainPage::iconButton()
{
    return ScriptNuiComponents::button(
        {.text = "Icon Button",
            .icon =
                Nui::Elements::Svg::svg{
                    Nui::Attributes::Svg::viewBox = "0 0 24 24"s,
                }(Nui::Elements::Svg::path{
                    Nui::Attributes::Svg::d = "m6 9 6 6 6-6"s,
                }()),
            .attributes = {
                Nui::Attributes::onClick = [](auto const&)
                {
                    Nui::WebApi::Console::log("Icon button clicked!");
                }
            }}
    );
}

Nui::ElementRenderer MainPage::colorPicker()
{
    return ScriptNuiComponents::colorPicker(
        ColorPickerOptions{
            .value = colorValue_,
            .pickButtonText = "",
            .pickButtonIcon =
                Nui::Elements::Svg::svg{
                    Nui::Attributes::Svg::viewBox = "0 0 512 512",
                }(Nui::Elements::Svg::path{
                    Nui::Attributes::Svg::d =
                        "M252 0q51 0 98 19.5T433 72t57.5 76.5T512 240q0 47-22.5 79.5T422 352H313q-21 0-29 10t-8 21q0 2 "
                        ".5 "
                        "7.5t3.5 11 9 10 17 4.5q20 0 33 13t13 31q0 14-9 25.5T322 502q-31 10-66 10-53 0-99.5-20.5T75 "
                        "436t-55-82-20-99q0-53 20.5-99.5t55-81 80-54.5T252 0zm170 301q22 0 "
                        "31-16.5t9-36.5q0-18-6.5-39t-17-41-22.5-37-24-29q-24-23-59.5-37T257 51q-43 0-80.5 16.5t-65.5 "
                        "44T67 "
                        "176t-16 79 16.5 79.5T112 400t65.5 44.5T256 461l19-1q-26-11-38.5-32T224 384q0-32 22.5-57.5T311 "
                        "301h111zM192 96q13 0 22.5 9t9.5 23-9.5 23-22.5 9q-14 0-23-9t-9-23 9-23 23-9zm128 0q13 0 22.5 "
                        "9t9.5 23-9.5 23-22.5 9q-14 0-23-9t-9-23 9-23 23-9zm-192 96q13 0 22.5 9t9.5 23-9 23-23 "
                        "9-23-9-9-23 "
                        "9.5-23 22.5-9zm256 0q13 0 22.5 9t9.5 23-9 23-23 9q-13 0-22.5-9t-9.5-23 9.5-23 22.5-9z",
                }()),
            .buttonStyleVariant = StyleVariant::Transparent,
        }
    );
}

void MainPage::rebuildFilterPills()
{
    std::vector<PillOptions> pills;
    pills.reserve(pillLabels_.size());
    for (auto const& label : pillLabels_)
    {
        pills.push_back(PillOptions{
            .text = label,
            .selected = pillSelected_.contains(label),
            .onClick =
                [this, label]()
            {
                if (pillSelected_.contains(label))
                    pillSelected_.erase(label);
                else
                    pillSelected_.insert(label);
                rebuildFilterPills();
            },
            .onRemove =
                [this, label]()
            {
                std::erase(pillLabels_, label);
                pillSelected_.erase(label);
                rebuildFilterPills();
            },
        });
    }
    *filterPills_ = std::move(pills);
}

Nui::ElementRenderer MainPage::pills()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    return div{style = "display: flex; flex-wrap: wrap; gap: 6px; align-items: center;"}(
        pill({.text = "Regular"}),
        pill({.text = "Selected", .selected = true}),
        pill({.text = "Clickable", .onClick = []() { Nui::WebApi::Console::log("Pill clicked"); }}),
        pill({.text = "Removable", .onRemove = []() { Nui::WebApi::Console::log("Pill removed"); }}),
        pill({.text = "With icon",
              .icon = Nui::Elements::Svg::svg{Nui::Attributes::Svg::viewBox = "0 0 24 24"}(
                  Nui::Elements::Svg::path{Nui::Attributes::Svg::d = "m6 9 6 6 6-6"}())})
    );
}

Nui::ElementRenderer MainPage::pillListSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using ScriptNuiComponents::button;

    return div{style = "display: flex; flex-direction: column; gap: 10px; align-items: flex-start;"}(
        pillList({.pills = filterPills_}),
        button({
            .text = "Add pill",
            .attributes = {
                onClick =
                    [this](auto const&)
                {
                    pillLabels_.push_back("tag-" + std::to_string(pillLabels_.size() + 1));
                    rebuildFilterPills();
                },
            },
        })
    );
}

Nui::ElementRenderer MainPage::tagBoxSection()
{
    using namespace Nui::Attributes;
    return tagBox_({style = "max-width: 420px;"});
}

Nui::ElementRenderer MainPage::toastSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using Nui::Elements::span;
    using ScriptNuiComponents::button;
    using ScriptNuiComponents::ToastPosition;
    using ScriptNuiComponents::ToastSeverity;
    using ScriptNuiComponents::ToastStyle;

    const auto styleButton = [this](std::string const& label, ToastStyle style) {
        return button({
            .text = label,
            .attributes = {onClick = [this, style](auto const&) { toast_.setStyle(style); }},
            .styleVariant = StyleVariant::Primary,
        });
    };

    const auto positionButton = [this](std::string const& label, ToastPosition position) {
        return button({
            .text = label,
            .attributes = {onClick = [this, position](auto const&) { toast_.setPosition(position); }},
        });
    };

    const auto severityButton = [this](std::string const& label, ToastSeverity severity, StyleVariant variant) {
        return button({
            .text = label,
            .attributes = {onClick =
                               [this, label, severity](auto const&)
                           {
                               toast_.show({
                                   .message = label + " toast: the operation reported something worth seeing.",
                                   .title = label,
                                   .severity = severity,
                               });
                           }},
            .styleVariant = variant,
        });
    };

    const auto row = [](std::string const& label, Nui::ElementRenderer buttons) {
        return div{style = "display: flex; align-items: center; gap: 6px; flex-wrap: wrap;"}(
            span{style = "min-width: 70px; opacity: 0.7;"}(label),
            std::move(buttons)
        );
    };

    return div{style = "display: flex; flex-direction: column; gap: 10px;"}(
        row("Style",
            div{style = "display: flex; gap: 6px;"}(
                styleButton("Box", ToastStyle::Box),
                styleButton("Strip", ToastStyle::Strip)
            )),
        row("Position",
            div{style = "display: flex; gap: 6px; flex-wrap: wrap;"}(
                positionButton("Top left", ToastPosition::TopLeft),
                positionButton("Top center", ToastPosition::TopCenter),
                positionButton("Top right", ToastPosition::TopRight),
                positionButton("Bottom left", ToastPosition::BottomLeft),
                positionButton("Bottom center", ToastPosition::BottomCenter),
                positionButton("Bottom right", ToastPosition::BottomRight)
            )),
        row("Show",
            div{style = "display: flex; gap: 6px; flex-wrap: wrap;"}(
                severityButton("Info", ToastSeverity::Info, StyleVariant::Primary),
                severityButton("Success", ToastSeverity::Success, StyleVariant::Success),
                severityButton("Warning", ToastSeverity::Warning, StyleVariant::Warning),
                severityButton("Error", ToastSeverity::Error, StyleVariant::Danger)
            )),
        row("Other",
            div{style = "display: flex; gap: 6px; flex-wrap: wrap;"}(
                button({
                    .text = "Sticky",
                    .attributes = {onClick =
                                       [this](auto const&)
                                   {
                                       toast_.show({
                                           .message = "Stays until dismissed, because its duration is zero.",
                                           .title = "Sticky",
                                           .severity = ToastSeverity::Warning,
                                           .durationMilliseconds = 0,
                                       });
                                   }},
                }),
                button({
                    .text = "Burst of 6",
                    .attributes = {onClick =
                                       [this](auto const&)
                                   {
                                       for (int index = 1; index <= 6; ++index)
                                       {
                                           toast_.info(fmt::format("Message number {} of the burst", index));
                                       }
                                   }},
                }),
                button({
                    .text = "Dismiss all",
                    .attributes = {onClick = [this](auto const&) { toast_.dismissAll(); }},
                    .styleVariant = StyleVariant::Transparent,
                })
            ))
    );
}

Nui::ElementRenderer MainPage::collapsibleSections()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    return div{style = "display: flex; flex-direction: column; gap: 4px; max-width: 480px;"}(
        collapsibleSection(
            {.title = "Today", .badge = "3"},
            div{}(
                div{}("git pull origin main"),
                div{}("docker compose up -d --build"),
                div{}("systemctl restart nginx")
            )
        ),
        collapsibleSection(
            {.title = "Yesterday", .badge = "2", .initiallyExpanded = false},
            div{}(
                div{}("kubectl get pods -n production"),
                div{}("df -h")
            )
        ),
        collapsibleSection(
            {.title = "Pinned",
             .initiallyExpanded = false,
             .onToggle = [](bool expanded) { Nui::WebApi::Console::log("Pinned expanded: ", expanded); }},
            div{}("htop")
        )
    );
}