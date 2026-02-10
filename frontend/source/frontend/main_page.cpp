#include <frontend/main_page.hpp>

#include <script-nui-components/switch.hpp>
#include <script-nui-components/text_input.hpp>

#include <nui/frontend/elements.hpp>

#include <vector>
#include <numeric>

using namespace std::string_literals;

MainPage::MainPage()
    : massSwitches()
{}

Nui::ElementRenderer MainPage::render()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using namespace ScriptNuiComponents;
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
            button{
                onClick = [this](auto const&) {
                    massSwitches.resize(1000);
                    std::iota(massSwitches.begin(), massSwitches.end(), 0);
                }
            }("Create 1000 switches"),
            button{
                onClick = [this](auto const&) {
                    massInputs.resize(1000);
                    std::iota(massInputs.begin(), massInputs.end(), 0);
                }
            }("Create 1000 inputs")                
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
            )
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