#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/element_renderer.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ScriptNuiComponents
{
    class Tabs
    {
      public:
        using OnSelect = std::function<bool /* really do select? */ (int id)>;
        using OnClose = std::function<bool /* remove it? */ (int id)>;
        using OnReorder = std::function<void(int from, int to)>;

      public:
        Tabs();
        ~Tabs();

        Tabs(Tabs const&) = delete;
        Tabs& operator=(Tabs const&) = delete;
        Tabs(Tabs&&);
        Tabs& operator=(Tabs&&);

        // external control
        void select(int id);
        void remove(int id);
        int add(std::string title, bool closable);

        void onSelect(OnSelect cb);
        void onClose(OnClose cb);
        void onReorder(OnReorder cb);

        Nui::ElementRenderer operator()();

        int makeId() const;

      private:
        void doReorder(int from, int insertAt);

        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}