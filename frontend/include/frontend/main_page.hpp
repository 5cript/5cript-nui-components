
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
#include <script-nui-components/progress_bar.hpp>

#include <cstdint>
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
    Nui::ElementRenderer checkboxSection();
    Nui::ElementRenderer textInput();
    Nui::ElementRenderer select();
    Nui::ElementRenderer labeledSelect();
    Nui::ElementRenderer iconButton();
    Nui::ElementRenderer colorPicker();
    Nui::ElementRenderer messageStrips();
    Nui::ElementRenderer pills();
    Nui::ElementRenderer pillListSection();
    Nui::ElementRenderer tagBoxSection();
    Nui::ElementRenderer collapsibleSections();
    Nui::ElementRenderer toastSection();

    /**
     * @brief Reactive and imperative progress bars in every text and color mode, see main_page_progress.cpp.
     */
    Nui::ElementRenderer progressBarSection();

    /**
     * @brief Progress rings: hue, theme and indeterminate, see main_page_progress.cpp.
     */
    Nui::ElementRenderer progressRingSection();

    /**
     * @brief Moves the progress bars and rings by an eighth, wrapping to 0 after complete.
     */
    void stepProgress();

    /**
     * @brief Badges in every variant, dots, a count badge and the pop animation, see main_page_indicators.cpp.
     */
    Nui::ElementRenderer badgeSection();

    /**
     * @brief A tab strip with count badges, see main_page_indicators.cpp.
     */
    Nui::ElementRenderer tabStripSection();

    /**
     * @brief Selectable list rows with leading, side text and trailing parts, see main_page_indicators.cpp.
     */
    Nui::ElementRenderer listRowSection();

    /**
     * @brief Collapsible sections with a header end slot and message strips with actions.
     */
    Nui::ElementRenderer slotExtensionsSection();

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

    std::shared_ptr<Nui::Observed<std::int64_t>> progressCurrent_{std::make_shared<Nui::Observed<std::int64_t>>(0)};
    std::shared_ptr<Nui::Observed<std::int64_t>> progressMaximum_{
        std::make_shared<Nui::Observed<std::int64_t>>(5'000'000)};
    std::shared_ptr<Nui::Observed<std::int64_t>> progressZero_{std::make_shared<Nui::Observed<std::int64_t>>(0)};
    std::shared_ptr<Nui::Observed<double>> ringFraction_{std::make_shared<Nui::Observed<double>>(0.0)};
    Nui::Observed<bool> ringIndeterminate_{true};

    Nui::Observed<int> badgeCount_{3};
    Nui::Observed<int> badgePop_{0};
    std::shared_ptr<Nui::Observed<std::string>> selectedTab_{std::make_shared<Nui::Observed<std::string>>("conflicts")};
    Nui::Observed<int> selectedRow_{0};

    ScriptNuiComponents::ResizableTable table_;
    ScriptNuiComponents::Dialog dialog_;
    ScriptNuiComponents::Tabs tabs_;
    ScriptNuiComponents::PopupMenu popupMenu_;
    ScriptNuiComponents::TagBox tagBox_;
    ScriptNuiComponents::Toast toast_;
    ScriptNuiComponents::ProgressBar imperativeBar_{{
        .height = "11px",
        .min = 0,
        .max = 5'000'000,
        .showMinMax = true,
        .byteMode = true,
    }};
};