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
        Switch{}(Switch::Options<decltype(isChecked_)>{
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

:root {
    /* Size factor for the switch, where 1 is the default size. This scales the entire switch, including the thumb. */
    --script-nui-components-switch-size-factor: 1;

    /* Colors for the switch. The active color is used when the switch is on, and the inactive color is used when the switch is off. The thumb color is used for the circular thumb of the switch. */
    --script-nui-components-switch-color-active: #008000;
    --script-nui-components-switch-color-inactive: #1f2228;
    --script-nui-components-switch-thumb-color: #dddddd;
}

### Text Input

#### C++

```cpp
#include <script-nui-components/text_input.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // value is Nui::Observed<std::string>, std::shared_ptr<Nui::Observed<std::string>>, or std::string.

    return body{}(
        TextInput{}(TextInput::Options<decltype(value_)>{
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
        Select{}(Select::Options<decltype(selectValue_), decltype(selectOptions_)>{
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