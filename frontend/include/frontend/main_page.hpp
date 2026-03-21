
#pragma once

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <script-nui-components/resizeable_table.hpp>
#include <script-nui-components/dialog.hpp>
#include <script-nui-components/tabs.hpp>
#include <script-nui-components/popup_menu.hpp>

class MainPage
{
  public:
    MainPage();

    ~MainPage()
    {
        Nui::WebApi::Console::log("MainPage destroyed");
    }

    Nui::ElementRenderer render();

  private:
    Nui::ElementRenderer switch_();
    Nui::ElementRenderer textInput();
    Nui::ElementRenderer select();
    Nui::ElementRenderer iconButton();
    Nui::ElementRenderer colorPicker();
    Nui::ElementRenderer messageStrips();

    Nui::ElementRenderer section(std::string const& title, Nui::ElementRenderer content);

    Nui::Observed<bool> isChecked_{false};
    Nui::Observed<std::string> textInputValue_{};
    Nui::Observed<std::string> colorValue_{};
    Nui::Observed<std::string> buttonTextSwitcher_{"A"};
    Nui::Observed<std::string> selectValue_{};

    std::vector<std::string> selectOptions_{std::vector<std::string>{
        "Option 1",
        "Option 2",
        "Option 3",
    }};

    ScriptNuiComponents::ResizableTable table_;
    ScriptNuiComponents::Dialog dialog_;
    ScriptNuiComponents::Tabs tabs_;
    ScriptNuiComponents::PopupMenu popupMenu_;
};