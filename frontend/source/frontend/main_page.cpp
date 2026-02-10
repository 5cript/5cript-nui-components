#include <frontend/main_page.hpp>

#include <script-nui-components/switch.hpp>

#include <nui/frontend/elements.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace ScriptNuiComponents;
    using Nui::Elements::div; // because of the global div.

    return body{}(Switch{}(Switch::Options<decltype(isChecked_), bool>{
        .isChecked = isChecked_,
        .sizeFactor = 1,
        .colorActive = "#008000",
        .onChange = [this](bool isChecked, Nui::WebApi::MouseEvent const&)
        {
            Nui::WebApi::Console::log(fmt::format("Switch is now {}", isChecked));
        }
    }));
}