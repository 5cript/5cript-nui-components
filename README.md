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
        Switch{}(Switch::Options<decltype(isChecked_), bool>{
            .isChecked = isChecked_,
            .isDisabled = false,
            .sizeFactor = 1,
            .colorActive = "#008000",
            .colorInactive = std::nullopt,
            .thumbColor = std::nullopt,
            .onChange = [this](bool isChecked, Nui::WebApi::MouseEvent const&)
            {
                Nui::WebApi::Console::log(fmt::format("Switch is now {}", isChecked));
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

## Styling

The components use CSS variables for styling, which can be set on the component itself or globally.