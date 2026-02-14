#include <frontend/main_page.hpp>

#include <script-nui-components/switch.hpp>
#include <script-nui-components/text_input.hpp>
#include <script-nui-components/select.hpp>
#include <script-nui-components/button.hpp>
#include <script-nui-components/color_picker.hpp>
#include <script-nui-components/resizeable_table.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <numeric>

using namespace std::string_literals;
using namespace ScriptNuiComponents;

MainPage::MainPage()
    : massSwitches()
    , table_{
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
          /*std::nullopt,*/
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
                          // std::function must be copiable
                          std::shared_ptr<ResizableTable::ISelfController> sharedController = std::move(controller);

                          return Button{}(Button::Options{
                              .icon =
                                  Nui::Elements::Svg::svg{
                                      Nui::Attributes::Svg::viewBox = "0 0 512 512",
                                  }(Nui::Elements::Svg::path{
                                      Nui::Attributes::Svg::d =
                                          "M454 109q11 0 18.5 7t7.5 18-7.5 18.5T454 160h-19v294q0 24-17 41t-41 "
                                          "17H135q-24 0-41-17t-17-41V160H58q-11 0-18.5-7.5T32 134t7.5-18 "
                                          "18.5-7h70V58q0-24 17-41t41-17h140q24 0 41 17t17 41v51h70zm-275 "
                                          "0h154V58q0-7-7-7H186q-7 0-7 7v51zm205 51H128v294q0 7 7 7h242q7 0 "
                                          "7-7V160zm-186 64q11 0 18.5 7.5T224 250v140q0 11-7.5 18.5T198 "
                                          "416t-18-7.5-7-18.5V250q0-11 7-18.5t18-7.5zm116 0q11 0 18 7.5t7 18.5v140q0 "
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
                              }
                          });
                      },
                  });
              },
              .addNewEntryText = "Add new row",
          }
      }
{}

Nui::ElementRenderer MainPage::render()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    // clang-format off
    return body{}(
        // Top bar
        div{class_ = "top-bar"}(
            Switch{}(Switch::Options<bool>{
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

            Button{}(Button::Options{
                .text = "Create 1000 switches",
                .attributes = {
                    onClick = [this](auto const&) {
                        massSwitches.resize(1000);
                    }
                }
            }),
            Button{}(Button::Options{
                .text = "Create 1000 inputs",
                .attributes = {
                    onClick = [this](auto const&) {
                        massInputs.resize(1000);
                    }
                },
                .styleVariant = StyleVariant::Primary,
            }),
            Button{}(Button::Options{
                .text = "Create 1000 selects",
                .attributes = {
                    onClick = [this](auto const&) {
                        massSelects.resize(1000);
                    }
                },
                .styleVariant = StyleVariant::Warning,
            }),
            Button{}(Button::Options{
                .text = "Create 1000 iconButtons",
                .attributes = {
                    onClick = [this](auto const&) {
                        massIconButtons.resize(1000);
                    }
                },
                .styleVariant = StyleVariant::Danger,
            }),
            Button{}(Button::Options{
                .text = "Create 1000 colorPickers",
                .attributes = {
                    onClick = [this](auto const&) {
                        massColorPickers.resize(1000);
                    }
                },
                .styleVariant = StyleVariant::Primary,
            })
        ),
        div{class_ = "content"}(
            div{}(
                range(massSwitches),
                [this](long long, auto) {
                    return switch_();
                }
            ),
            div{}(
                range(massInputs),
                [this](long long, auto) {
                    return textInput();
                }
            ),
            div{}(
                range(massSelects),
                [this](long long, auto) {
                    return this->select();
                }
            ),
            div{}(
                range(massIconButtons),
                [this](long long, auto) {
                    return iconButton();
                }
            ),
            div{}(
                range(massColorPickers),
                [this](long long, auto) {
                    return colorPicker();
                }
            )
        ),
        div{style = "margin-top: 20px; width: 100%; display: flex"}(
            table_({
                style = "max-height: 400px"
            })
        )
    );
    // clang-format on
}

Nui::ElementRenderer MainPage::switch_()
{
    using namespace ScriptNuiComponents;

    return Switch{}(Switch::Options<decltype(isChecked_)>{
        .isChecked = isChecked_,
    });
}

Nui::ElementRenderer MainPage::textInput()
{
    using namespace ScriptNuiComponents;

    return TextInput{}(TextInput::Options<decltype(textInputValue_)>{
        .value = textInputValue_,
    });
}

Nui::ElementRenderer MainPage::select()
{
    using namespace ScriptNuiComponents;

    return Select{}(Select::Options<decltype(selectValue_), decltype(selectOptions_)>{
        .activeOption = selectValue_,
        .options = selectOptions_,
        .attributes =
            {
                Nui::Attributes::disabled = true,
            },
        .onChange = [](std::string const& newValue, auto const&)
        {
            Nui::WebApi::Console::log("Selected option: "s + newValue);
        }
    });
}

Nui::ElementRenderer MainPage::iconButton()
{
    using namespace ScriptNuiComponents;

    return Button{}(Button::Options{
        .text = "Icon Button",
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
        }
    });
}

Nui::ElementRenderer MainPage::colorPicker()
{
    using namespace ScriptNuiComponents;

    return ColorPicker{}(ColorPicker::Options<decltype(colorValue_)>{
        .value = colorValue_,
        .pickButtonText = "",
        .pickButtonIcon =
            Nui::Elements::Svg::svg{
                Nui::Attributes::Svg::viewBox = "0 0 512 512",
            }(Nui::Elements::Svg::path{
                Nui::Attributes::Svg::d =
                    "M252 0q51 0 98 19.5T433 72t57.5 76.5T512 240q0 47-22.5 79.5T422 352H313q-21 0-29 10t-8 21q0 2 .5 "
                    "7.5t3.5 11 9 10 17 4.5q20 0 33 13t13 31q0 14-9 25.5T322 502q-31 10-66 10-53 0-99.5-20.5T75 "
                    "436t-55-82-20-99q0-53 20.5-99.5t55-81 80-54.5T252 0zm170 301q22 0 "
                    "31-16.5t9-36.5q0-18-6.5-39t-17-41-22.5-37-24-29q-24-23-59.5-37T257 51q-43 0-80.5 16.5t-65.5 44T67 "
                    "176t-16 79 16.5 79.5T112 400t65.5 44.5T256 461l19-1q-26-11-38.5-32T224 384q0-32 22.5-57.5T311 "
                    "301h111zM192 96q13 0 22.5 9t9.5 23-9.5 23-22.5 9q-14 0-23-9t-9-23 9-23 23-9zm128 0q13 0 22.5 "
                    "9t9.5 23-9.5 23-22.5 9q-14 0-23-9t-9-23 9-23 23-9zm-192 96q13 0 22.5 9t9.5 23-9 23-23 9-23-9-9-23 "
                    "9.5-23 22.5-9zm256 0q13 0 22.5 9t9.5 23-9 23-23 9q-13 0-22.5-9t-9.5-23 9.5-23 22.5-9z",
            }()),
        .buttonStyleVariant = StyleVariant::Transparent,
    });
}