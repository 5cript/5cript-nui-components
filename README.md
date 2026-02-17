# 5cript-nui-components

Reusable components that I use with NuiCpp.
These have been created out of frustration with horribly performing component libs.
Not part of NuiCpp because I am not upholding the quality bar for nui here, nor do I strive for completeness, just what I need for my projects.
Also I am not a great ui designer.

## Use in project

CMake:
```cmake
include(FetchContent)

FetchContent(
	script-nui-components
	GIT_REPOSITORY https://github.com/NuiCpp/Nui.git
	GIT_TAG        v2.0.0
)
FetchContent_MakeAvailable(script-nui-components)

target_link_libraries(your_target PRIVATE script-nui-components)
```

## Styling

The components use CSS variables for styling, which can be set on the component itself or globally.

## Components

### Switch

#### C++

```cpp
#include <script-nui-components/switch.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // isChecked_ is Nui::Observed<bool>, std::shared_ptr<Nui::Observed<bool>>, or bool.

    return body{}(
        switch_(SwitchOptions<decltype(isChecked_)>{
            .isChecked = isChecked_,
            .sizeFactor = 1,
            .colorActive = "#008000",
            .colorInactive = std::nullopt,
            .thumbColor = std::nullopt,
            .onChange = [this](bool isChecked, Nui::WebApi::MouseEvent const&)
            {
                Nui::WebApi::Console::log(fmt::format("Switch is now {}", isChecked));
            },
            .attributes = {
                // and others...
                .disabled = false
            }
        })
    );
}
```

#### Css Variables

```css
:root {
    /* Size factor for the switch, where 1 is the default size. This scales the entire switch, including the thumb. */
    --script-nui-components-switch-size-factor: 1;

    /* Colors for the switch. The active color is used when the switch is on, and the inactive color is used when the switch is off. The thumb color is used for the circular thumb of the switch. */
    --script-nui-components-switch-color-active: #008000;
    --script-nui-components-switch-color-inactive: #1f2228;
    --script-nui-components-switch-thumb-color: #dddddd;
}
```

### Button

#### C++

```cpp
#include <script-nui-components/button.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    return body{}(
        return button(ButtonOptions{
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
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-button-color-primary: #5fbcff;
    --script-nui-components-button-color-warning: #fbbf24;
    --script-nui-components-button-color-danger: #dc2626;
    --script-nui-components-button-color-regular: #202020;
    --script-nui-components-button-color-disabled: #505050;
    --script-nui-components-button-border-color: #505050;
    --script-nui-components-button-color: #dddddd;
    --script-nui-components-button-font: inherit;
    --script-nui-components-button-font-size: 0.8rem;
    --script-nui-components-button-border-radius: 0px;
}
```

### Text Input

#### C++

```cpp
#include <script-nui-components/text_input.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // value is Nui::Observed<std::string>, std::shared_ptr<Nui::Observed<std::string>>, or std::string.

    return body{}(
        textInput(TextInputOptions<decltype(value_)>{
            .value = value_,
            .onChange = [this](std::string const& newValue, Nui::WebApi::InputEvent const&)
            {
                Nui::WebApi::Console::log(fmt::format("Text input is now {}", newValue));
            },
            .attributes = {
                // and others...
                .disabled = false
            }
        })
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-field-border-radius: 0px;
    --script-nui-components-field-color: #dddddd;
    --script-nui-components-field-background-color: #202020;
    --script-nui-components-field-font: inherit;
    --script-nui-components-field-font-size: 0.8rem;
    --script-nui-components-field-theme-color: #5fbcff;
    --script-nui-components-field-hover-shadow: 0 0 0 0.1rem var(--script-nui-components-field-theme-color);
    --script-nui-components-field-border-color: #505050;
    --script-nui-components-field-focus-shadow: var(--script-nui-components-field-hover-shadow);
    /* Used for invalid state of the input, e.g. when pattern attribute is not matched. */
    --script-nui-components-field-color-invalid: #dc2626;
}
```

### Select

#### C++

```cpp
#include <script-nui-components/select.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // MainPage members:
    //
    // Nui::Observed<std::string> selectValue_{};
    // Nui::Observed<std::vector<std::string>> selectOptions_{
    //     std::vector<std::string>{"Option 1", "Option 2", "Option 3"},
    // };

    return body{}(
        select(SelectOptions<decltype(selectValue_), decltype(selectOptions_)>{
            .activeOption = selectValue_,
            .options = selectOptions_,
            .attributes = {
                // and others...
                .disabled = false
            },
            .onChange = [this](std::string const& newValue, auto const&)
            {
                Nui::WebApi::Console::log("Selected option: "s + newValue);
            },
            // Creates ui elements active element, can be used to show icons or images in addition to text.
            // Default is a span.
            .activeRenderer = [](Nui::Observed<std::string> const& option) -> Nui::ElementRenderer
            {
                return span{}(option);
            },
            // Creates ui elements from vector elements, can be used to show icons or images in addition to text.
            // Default is a span.
            .elementRenderer = [](std::string const& option) -> Nui::ElementRenderer
            {
                return span{}(option);
            }
        })
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-field-border-radius: 0px;
    --script-nui-components-field-color: #dddddd;
    --script-nui-components-field-background-color: #202020;
    --script-nui-components-field-font: inherit;
    --script-nui-components-field-font-size: 0.8rem;
    --script-nui-components-field-theme-color: #5fbcff;
    --script-nui-components-field-hover-shadow: 0 0 0 0.1rem var(--script-nui-components-field-theme-color);
    --script-nui-components-field-border-color: #505050;
    --script-nui-components-field-focus-shadow: var(--script-nui-components-field-hover-shadow);
}
```

### Color Picker

#### C++

```cpp
#include <script-nui-components/color_picker.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // MainPage members:
    //
    // Nui::Observed<std::string> colorValue_{};

    return body{}(
        return colorPicker(ColorPickerOptions<decltype(colorValue_)>{
            .value = colorValue_,
            .pickButtonText = "",
            .pickButtonIcon = Nui::Elements::Svg::svg{
                Nui::Attributes::Svg::viewBox = "0 0 24 24",
            }(Nui::Elements::Svg::path{
                Nui::Attributes::Svg::d = "" /* something meaningful */,
            }()),
            .buttonStyleVariant = StyleVariant::Transparent
        });
    );
}
```

#### Css Variables

Styling is determined by button and text field styles.

### Resizeable Table

This component does not self holds its state and needs to be instantiated outside the render flow.
It does not hold the table model, but relies on the user to add / remove rows.

#### C++

```cpp
#include <script-nui-components/resizeable_table.hpp>

MainPage::MainPage()
    : table_{
          ResizableTable::HeaderRow{
              // Resizeable with dynamic width:
              ResizableTable::HeaderTableCell{"Header 1"s},
              ResizableTable::HeaderTableCell{"Header 2"s},
              ResizableTable::HeaderTableCell{
                  .content = "",
                  .initialWidth = 40,
                  .resizeable = false,
              }
          },
          // or nullopt for no footer
          ResizableTable::FooterFeature{{"Footer 1"s}, {"Footer 2"s}, {""s}},
          // or nullopt for no add row:
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

                          return button(ButtonOptions{
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
                                      // myState.erase(controller->row());
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
    using namespace ScriptNuiComponents;

    return body{}(
        return table_({
            .disabled = false
        })
    );
}
```

#### Css Variables

```c++
:root {
    --script-nui-table-border-radius: 10px;
    --script-nui-table-font-size: 0.85rem;
    --script-nui-table-font: system-ui, -apple-system, sans-serif;

    --script-nui-table-color: #c9d1d9;
    --script-nui-table-header-color: #e6edf3;
    --script-nui-table-footer-color: #8b949e;

    --script-nui-table-cell-background: rgb(30, 30, 30);
    --script-nui-table-header-background: linear-gradient(180deg, var(--script-nui-table-cell-background), color-mix(in srgb, var(--script-nui-table-cell-background), black 40%));
    --script-nui-table-footer-background: color-mix(in srgb, var(--script-nui-table-cell-background), white 10%);

    --script-nui-table-border-color: #444444;
    --script-nui-table-row-hover: color-mix(in srgb, var(--script-nui-table-cell-background), white 10%);

    --script-nui-table-handle-color: rgb(90, 90, 90);
    --script-nui-table-handle-active: #0078d4;
}
```