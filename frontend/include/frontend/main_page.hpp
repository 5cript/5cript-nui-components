#pragma once

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/element_renderer.hpp>

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

  private:
    Nui::Observed<std::vector<int>> massSwitches;
    Nui::Observed<std::vector<int>> massInputs;
    Nui::Observed<std::vector<int>> massSelects;

    Nui::Observed<bool> isChecked_{false};
    Nui::Observed<std::string> textInputValue_{};

    Nui::Observed<std::string> selectValue_{};
    Nui::Observed<std::vector<std::string>> selectOptions_{std::vector<std::string>{
        "Option 1",
        "Option 2",
        "Option 3",
    }};
};