#include <frontend/main_page.hpp>

#include <script-nui-components/switch.hpp>
#include <script-nui-components/text_input.hpp>
#include <script-nui-components/select.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

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
            }("Create 1000 inputs"),
            button{
                onClick = [this](auto const&) {
                    massSelects.resize(1000);
                    std::iota(massSelects.begin(), massSelects.end(), 0);
                }
            }("Create 1000 selects")
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