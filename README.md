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