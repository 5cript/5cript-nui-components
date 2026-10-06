#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/element_renderer.hpp>

#include <functional>
#include <memory>
#include <string>
#include <optional>
#include <any>

namespace ScriptNuiComponents
{
    class Tabs
    {
      public:
        struct Tab
        {
            int id; // stable forever
            std::string title;
            bool closable = false;
            // User provided info attached to the tab:
            std::optional<std::any> metadata{std::nullopt};
        };

        /**
         * @brief CSS classes the tab bar renders with, so it can take on another tab stylesheet.
         */
        struct ClassNames
        {
            std::string bar = "script-nui-tab-bar";
            /**
             * @brief Wrapper around each tab and its leading drop marker.
             */
            std::string item = "";
            std::string tab = "script-nui-tab";
            /**
             * @brief Added to the tab class of the selected tab.
             */
            std::string selectedTab = "selected";
            std::string label = "";
            std::string closeButton = "";
        };

        using OnSelect = std::function<bool /* really do select? */ (int id)>;
        using OnClose = std::function<bool /* remove it? */ (int id)>;
        using OnReorder = std::function<void(int from, int to)>;

      public:
        Tabs();
        explicit Tabs(ClassNames classNames);
        ~Tabs();

        Tabs(Tabs const&) = delete;
        Tabs& operator=(Tabs const&) = delete;
        Tabs(Tabs&&);
        Tabs& operator=(Tabs&&);

        // external control
        void select(int id);
        void remove(int id);
        int add(std::string title, bool closable, std::optional<std::any> metadata = std::nullopt);

        void onSelect(OnSelect cb);
        void onClose(OnClose cb);
        void onReorder(OnReorder cb);

        Tab* getById(int id);
        Tab* getSelected();
        int selectedId() const;
        int firstTabId() const;

        void modifyTabById(int id, std::function<void(Tab*)> modifyFn);

        Nui::ElementRenderer
        operator()(std::vector<Nui::Attribute> extraAttributes = {}, std::vector<std::string> const& extraClasses = {});

        int makeId() const;

      private:
        void doReorder(int from, int insertAt);

        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}