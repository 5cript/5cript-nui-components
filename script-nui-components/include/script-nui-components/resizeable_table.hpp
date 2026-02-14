#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <variant>
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ScriptNuiComponents
{
    class ResizableTable
    {
      public:
        using AddCallback = std::function<void(ResizableTable&)>;

        struct ISelfController
        {
            virtual ~ISelfController() = default;
            virtual void remove() = 0;
            virtual int row() const = 0;
            virtual int column() const = 0;
        };

        using TableCell =
            std::variant<std::string, std::function<Nui::ElementRenderer(std::unique_ptr<ISelfController> controller)>>;

        // Inheritance destroys the ability to use aggregate initialization, so we use a separate struct for header
        // cells
        struct HeaderTableCell
        {
            std::variant<std::string, std::function<Nui::ElementRenderer(std::unique_ptr<ISelfController> controller)>>
                content;
            std::optional<int> initialWidth{0};
            bool resizeable{true};
        };

        using TableRow = std::vector<TableCell>;
        using HeaderRow = std::vector<HeaderTableCell>;

        struct AddFeature
        {
            AddCallback onAdd;
            std::string addNewEntryText = "Add new Entry";
        };
        using FooterFeature = TableRow;

        ResizableTable(
            HeaderRow header,
            std::optional<FooterFeature> footer,
            std::optional<AddFeature> addFeature = std::nullopt
        );
        ~ResizableTable();

        ResizableTable(ResizableTable const&) = delete;
        ResizableTable& operator=(ResizableTable const&) = delete;
        ResizableTable(ResizableTable&&);
        ResizableTable& operator=(ResizableTable&&);

        Nui::ElementRenderer operator()(std::vector<Nui::Attribute>&& attributes = {});

        void addRow(TableRow row);
        void setRows(std::vector<TableRow> rows);
        void remove(std::size_t index);
        void clear();
        int rowCount() const;
        int columnCount() const;

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}