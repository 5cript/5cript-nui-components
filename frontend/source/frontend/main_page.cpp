#include <frontend/main_page.hpp>

#include <script-nui-components/switch.hpp>

#include <nui/frontend/elements.hpp>

#include <vector>
#include <numeric>

using namespace std::string_literals;

MainPage::MainPage()
    : massCreation()
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
            Switch{}(Switch::Options<bool, bool>{
                .isChecked = true,
                .onChange =
                    [this](bool isChecked, Nui::WebApi::MouseEvent const&)
                {
                    if (isChecked)
                        Nui::val::global("document")["documentElement"].call<void>("setAttribute", "data-theme"s, "dark"s);
                    else
                        Nui::val::global("document")["documentElement"].call<void>("setAttribute", "data-theme"s, "light"s);
                },
            }),
            button{
                onClick = [this](auto const&) {
                    massCreation.resize(1000);
                    std::iota(massCreation.begin(), massCreation.end(), 0);
                }
            }("Create 1000 switches")                
        ),
        div{class_ = "content"}(
            range(massCreation),
            [this](long long i, auto) {
                return switch_();
            }
        )
    );
    // clang-format on
}

Nui::ElementRenderer MainPage::switch_()
{
    using namespace ScriptNuiComponents;

    return Switch{}(Switch::Options<decltype(isChecked_), bool>{.isChecked = isChecked_});
}