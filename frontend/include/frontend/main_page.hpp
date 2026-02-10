#pragma once

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/element_renderer.hpp>

class MainPage
{
  public:
    Nui::ElementRenderer render();
    ~MainPage()
    {
        Nui::WebApi::Console::log("MainPage destroyed");
    }

  private:
    // std::shared_ptr<Nui::Observed<bool>> isChecked_{std::make_shared<Nui::Observed<bool>>(false)};
    // Nui::Observed<bool> isChecked_{false};
    bool isChecked_ = false;
};