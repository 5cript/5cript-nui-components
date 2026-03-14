#pragma once

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <script-nui-components/resizeable_table.hpp>
#include <script-nui-components/dialog.hpp>
#include <script-nui-components/tabs.hpp>

class MainPage
{
  public:
    MainPage();

    ~MainPage()
    {
        Nui::WebApi::Console::log("MainPage destroyed");
    }

    Nui::ElementRenderer render();
    Nui::ElementRenderer switch_();
    Nui::ElementRenderer textInput();
    Nui::ElementRenderer select();
    Nui::ElementRenderer iconButton();
    Nui::ElementRenderer colorPicker();

  private:
    Nui::Observed<std::vector<int>> massSwitches;
    Nui::Observed<std::vector<int>> massInputs;
    Nui::Observed<std::vector<int>> massSelects;
    Nui::Observed<std::vector<int>> massIconButtons;
    Nui::Observed<std::vector<int>> massColorPickers;

    Nui::Observed<bool> isChecked_{false};
    Nui::Observed<std::string> textInputValue_{};
    Nui::Observed<std::string> colorValue_{};

    Nui::Observed<std::string> selectValue_{};
    // Nui::Observed<std::vector<std::string>> selectOptions_{std::vector<std::string>{
    //     "Option 1",
    //     "Option 2",
    //     "Option 3",
    // }};
    std::vector<std::string> selectOptions_{std::vector<std::string>{
        "Option 1",
        "Option 2",
        "Option 3",
    }};

    ScriptNuiComponents::ResizableTable table_;
    ScriptNuiComponents::Dialog dialog_;
    ScriptNuiComponents::Tabs tabs_;
};