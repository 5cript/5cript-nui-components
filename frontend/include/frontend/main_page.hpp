
#pragma once

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>
#include <script-nui-components/resizeable_table.hpp>
#include <script-nui-components/dialog.hpp>
#include <script-nui-components/tabs.hpp>
#include <script-nui-components/popup_menu.hpp>
#include <script-nui-components/pill.hpp>
#include <script-nui-components/pill_list.hpp>
#include <script-nui-components/tag_box.hpp>
#include <script-nui-components/toast.hpp>

#include <memory>
#include <set>
#include <string>
#include <vector>

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
    Nui::ElementRenderer pills();
    Nui::ElementRenderer pillListSection();
    Nui::ElementRenderer tagBoxSection();
    Nui::ElementRenderer collapsibleSections();
    Nui::ElementRenderer toastSection();

    /// Rebuilds the observed pill list from pillLabels_/pillSelected_ (the demo's source of truth).
    void rebuildFilterPills();

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

    // Pill list demo: labels + selection are the source of truth; filterPills_ is derived.
    std::vector<std::string> pillLabels_{"docker", "logs", "ops", "k8s"};
    std::set<std::string> pillSelected_{"docker"};
    std::shared_ptr<Nui::Observed<std::vector<ScriptNuiComponents::PillOptions>>> filterPills_{
        std::make_shared<Nui::Observed<std::vector<ScriptNuiComponents::PillOptions>>>()};

    ScriptNuiComponents::ResizableTable table_;
    ScriptNuiComponents::Dialog dialog_;
    ScriptNuiComponents::Tabs tabs_;
    ScriptNuiComponents::PopupMenu popupMenu_;
    ScriptNuiComponents::TagBox tagBox_;
    ScriptNuiComponents::Toast toast_;
};