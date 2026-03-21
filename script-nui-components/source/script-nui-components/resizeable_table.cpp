#include <script-nui-components/resizeable_table.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/val.hpp>
#include <nui/frontend/api/abort_controller.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/dom/basic_element.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>

#include <fmt/format.h>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;
using namespace std::string_literals;

namespace ScriptNuiComponents
{
    struct TableRowWithSelfController
    {
        ResizableTable::TableRow row;
        std::unique_ptr<ResizableTable::ISelfController> selfController;
    };

    struct ResizableTable::Implementation
    {
        HeaderRow header;
        std::optional<FooterFeature> footer;
        Observed<std::vector<TableRowWithSelfController>> rows;
        Observed<std::vector<std::optional<int>>> columnWidths;
        Observed<bool> wasResizedOnce{false};
        std::weak_ptr<Nui::Dom::BasicElement> reference;
        std::optional<ResizableTable::AddFeature> addFeature;

        Nui::val window = Nui::val::global("window");
        Nui::WebApi::AbortController mouseMoveAbort;

        Implementation(
            HeaderRow head,
            std::optional<ResizableTable::FooterFeature> foot,
            std::optional<ResizableTable::AddFeature> addFeature
        )
            : header{std::move(head)}
            , footer{std::move(foot)}
            , rows{}
            , columnWidths{[this]()
                  {
                      std::vector<std::optional<int>> widths;
                      if (!header.empty())
                          widths.resize(header.size(), std::nullopt);
                      for (std::size_t i = 0; i < widths.size(); ++i)
                      {
                          if (header[i].initialWidth.has_value())
                              widths[i] = header[i].initialWidth;
                      }
                      return widths;
                  }()}
            , addFeature{std::move(addFeature)}
        {
            if (footer && header.size() != footer->size())
            {
                WebApi::Console::error(
                    "Header and footer must have the same number of columns. Header has",
                    header.size(),
                    "but footer has",
                    footer->size()
                );
            }
        }

        void startResizeImpl(std::size_t colIndex, Nui::WebApi::MouseEvent const& event)
        {
            mouseMoveAbort.abort(); // cancel any previous drag
            mouseMoveAbort = {};

            auto activeColumn = colIndex;
            auto resizeStartX = event.clientX();
            auto resizeStartWidth = (*columnWidths)[colIndex].value_or(100);

            auto options = Nui::val::object();
            options.set("signal", mouseMoveAbort.signal().val());

            Nui::val::global("window").call<void>(
                "addEventListener",
                "mousemove"s,
                Nui::bind(
                    // Nui::val, bind does not accept WebApi::Event classes:
                    [this, activeColumn, resizeStartX, resizeStartWidth](Nui::val v)
                    {
                        auto event = Nui::WebApi::MouseEvent{std::move(v)};
                        int dx = event.clientX() - resizeStartX;
                        columnWidths[activeColumn] = std::max(40, resizeStartWidth + dx);

                        Nui::globalEventContext.executeActiveEventsImmediately();
                    },
                    std::placeholders::_1
                ),
                options
            );

            Nui::val::global("window").call<void>(
                "addEventListener",
                "mouseup"s,
                Nui::bind(
                    // Nui::val, bind does not accept WebApi::Event classes:
                    [this](Nui::val)
                    {
                        mouseMoveAbort.abort();
                    },
                    std::placeholders::_1
                ),
                options
            );
        }

        void startResize(std::size_t colIndex, Nui::WebApi::MouseEvent event)
        {
            if (!wasResizedOnce)
            {
                // On the first resize, set all column widths to their current pixel width to enable resizing from there
                // on out. This is needed because before any resizing, the columns are sized by the browser based on
                // their content, and not by our columnWidths values.
                window.call<void>(
                    "requestAnimationFrame",
                    Nui::bind(
                        [this, colIndex, event = std::move(event)](Nui::val)
                        {
                            wasResizedOnce = true;
                            auto elem = reference.lock();
                            if (!elem)
                            {
                                WebApi::Console::error("Failed to get reference to table element for resizing.");
                                return;
                            }

                            // Get actual pixel widths of columns from the DOM and set them as the initial widths for
                            // resizing.
                            std::vector<int> actualWidths;
                            for (std::size_t i = 0; i < header.size(); ++i)
                            {
                                // Query the header cell specifically
                                auto thElement = elem->val().call<Nui::val>(
                                    "querySelector", fmt::format("thead tr th:nth-child({})", i + 1)
                                );

                                if (thElement.isNull() || thElement.isUndefined())
                                {
                                    actualWidths.push_back(100); // Fallback
                                    continue;
                                }

                                // offsetWidth is reliable on table cells.
                                // If you need sub-pixel precision, use:
                                int width = thElement.call<Nui::val>("getBoundingClientRect")["width"].as<int>();
                                actualWidths.push_back(width);
                            }
                            *columnWidths = std::vector<std::optional<int>>(actualWidths.begin(), actualWidths.end());
                            columnWidths.modify();
                            Nui::globalEventContext.executeActiveEventsImmediately();
                            startResizeImpl(colIndex, event);
                        },
                        std::placeholders::_1
                    )
                );
                return;
            }
            startResizeImpl(colIndex, event);
        }
    };

    namespace
    {
        class SelfController : public ResizableTable::ISelfController
        {
          public:
            SelfController(ResizableTable& parent, int row, int column)
                : parent_{&parent}
                , row_{row}
                , column_{column}
            {}

            int row() const override
            {
                return row_;
            }
            int column() const override
            {
                return column_;
            }
            void remove() override
            {
                parent_->remove(static_cast<std::size_t>(row_));
            }
            void row(int newRow)
            {
                row_ = newRow;
            }
            virtual ResizableTable::TableRow const& rowData() const override
            {
                return parent_->row(static_cast<std::size_t>(row_));
            }

          private:
            ResizableTable* parent_;
            int row_;
            int column_;
        };

        Nui::ElementRenderer
        renderCell(ResizableTable& table, ResizableTable::TableCell const& cell, int rowIndex, int colIndex)
        {
            using namespace Nui::Elements;

            if (std::holds_alternative<std::string>(cell))
                return td{}(std::get<std::string>(cell));

            return td{}(
                std::get<std::function<Nui::ElementRenderer(std::unique_ptr<ResizableTable::ISelfController>)>>(cell)(
                    std::make_unique<SelfController>(table, rowIndex, colIndex)
                )
            );
        }

        Nui::ElementRenderer
        renderCell(ResizableTable& table, ResizableTable::HeaderTableCell const& cell, int rowIndex, int colIndex)
        {
            using namespace Nui::Elements;

            if (std::holds_alternative<std::string>(cell.content))
                return td{}(std::get<std::string>(cell.content));

            return td{}(
                std::get<std::function<Nui::ElementRenderer(std::unique_ptr<ResizableTable::ISelfController>)>>(cell
                        .content)(std::make_unique<SelfController>(table, rowIndex, colIndex))
            );
        }
    }

    ResizableTable::ResizableTable(
        HeaderRow header,
        std::optional<FooterFeature> footer,
        std::optional<AddFeature> addFeature
    )
        : impl_{std::make_unique<Implementation>(std::move(header), std::move(footer), std::move(addFeature))}
    {}

    ResizableTable::~ResizableTable() = default;
    ResizableTable::ResizableTable(ResizableTable&&) = default;
    ResizableTable& ResizableTable::operator=(ResizableTable&&) = default;

    void ResizableTable::addRow(TableRow row)
    {
        if (row.size() != impl_->header.size())
        {
            Nui::WebApi::Console::error(
                "Attempted to add a row with a different number of columns than the header. Expected",
                impl_->header.size(),
                "but got",
                row.size()
            );
            return;
        }
        impl_->rows.push_back(
            TableRowWithSelfController{
                std::move(row), std::make_unique<SelfController>(*this, static_cast<int>(impl_->rows->size()), -1)
            }
        );
    }

    void ResizableTable::setRows(std::vector<TableRow> rows)
    {
        impl_->rows.clear();
        for (auto& row : rows)
        {
            addRow(std::move(row));
        }
    }

    void ResizableTable::remove(std::size_t index)
    {
        if (index >= impl_->rows->size())
            return;

        impl_->rows.erase(impl_->rows->begin() + index);

        // Update indices for shifted rows
        for (std::size_t i = index; i < impl_->rows->size(); ++i)
        {
            auto* controller = impl_->rows[i]->selfController.get();
            if (controller)
                static_cast<SelfController*>(controller)->row(static_cast<int>(i));
        }
    }

    void ResizableTable::clear()
    {
        impl_->rows.clear();
    }

    Nui::ElementRenderer ResizableTable::operator()(std::vector<Nui::Attribute>&& attributes)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        // clang-format off
        return div{
            mergeAttributes({
                class_ = "script-nui-resizable-table",
                reference = impl_->reference
            }, std::move(attributes))
        }(
            table{
                style = observe(impl_->wasResizedOnce).generate([&](bool wasResizedOnce) {
                    return wasResizedOnce ? "table-layout: fixed;" : "width: 100%";
                })
            }(
                colgroup{}(
                    Nui::range(impl_->columnWidths),
                    [](long long /*i*/, std::optional<int> width)
                    {
                        return col{
                            style = width ? std::optional<std::string>{fmt::format("width:{}px;", *width)} : std::nullopt
                        }();
                    }
                ),

                // ===== HEADER =====
                thead{}(
                    tr{}(
                        Nui::range(impl_->header),
                        [this](long long col, HeaderTableCell const& cell)
                        {
                            return th{
                                onMouseDown = [this, col, canResize = cell.resizeable](Nui::WebApi::MouseEvent event) {
                                    if (!canResize)
                                        return;

                                    impl_->startResize(
                                        static_cast<std::size_t>(col),
                                        std::move(event)
                                    );
                                }
                            }(
                                renderCell(*this, cell, -1, static_cast<int>(col)),
                                cell.resizeable ? div{class_ = "resize-handle"}() : Nui::nil()
                            );
                        }
                    )
                ),

                // ===== BODY =====
                tbody{}(
                    Nui::range(impl_->rows)
                        .after(
                            impl_->addFeature ?
                            tr{
                                class_ = "add-row-action",
                                onClick = [this](Nui::val) { impl_->addFeature->onAdd(*this); }
                            }(
                                td{
                                    "colspan"_attr = std::to_string(impl_->header.size()),
                                    style = "text-align: center; cursor: pointer;"
                                }(
                                    span{ style = "margin-right: 0.5em;" }("+"),
                                    text{impl_->addFeature->addNewEntryText}()
                                )
                            ) : Nui::nil()
                        ),
                    [this](long long rowIndex, TableRowWithSelfController const& row) {
                        return tr{}(
                            Nui::range(row.row),
                            [this, rowIndex = static_cast<int>(rowIndex)](long long col, TableCell const& cell) {
                                return renderCell(*this, cell, rowIndex, static_cast<int>(col));
                            }
                        );
                    }
                ),

                // ===== FOOTER =====
                impl_->footer ?
                tfoot{}(
                    tr{}(
                        Nui::range(*impl_->footer),
                        [this](long long col, TableCell const& cell)
                        {
                            return renderCell(*this, cell, -2, static_cast<int>(col));
                        }
                    )
                ) : Nui::nil()
            )
        );
        // clang-format on
    }

    int ResizableTable::rowCount() const
    {
        return impl_->rows->size();
    }
    int ResizableTable::columnCount() const
    {
        return impl_->header.size();
    }
    ResizableTable::TableRow const& ResizableTable::row(std::size_t index) const
    {
        return (*impl_->rows)[index].row;
    }
}