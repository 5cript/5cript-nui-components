#include <script-nui-components/tabs.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/event.hpp>
#include <nui/frontend/api/drag_event.hpp>
#include <nui/frontend/api/mouse_event.hpp>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;

namespace ScriptNuiComponents
{
    namespace
    {
        struct Tab
        {
            int id; // stable forever
            std::string title;
            bool closable = false;
        };
    }

    struct Tabs::Implementation
    {
        Observed<std::vector<Tab>> tabs{};
        Observed<int> selectedId{-1};
        int idCounter{0};

        OnSelect onSelect{};
        OnClose onClose{};
        OnReorder onReorder{};

        std::optional<int> dragSource;

        // Visual slot index where the drop-marker is shown.
        // 0 = before first tab, N = after Nth tab (0-based). -1 = hidden.
        Observed<int> dropInsertIndex{-1};
    };

    Tabs::Tabs()
        : impl_{std::make_unique<Implementation>()}
    {
        onClose(
            [this](int id)
            {
                remove(id);
                return true;
            }
        );
    }

    Tabs::~Tabs() = default;
    Tabs::Tabs(Tabs&&) = default;
    Tabs& Tabs::operator=(Tabs&&) = default;

    void Tabs::select(int id)
    {
        impl_->selectedId = id;
    }

    int Tabs::makeId() const
    {
        return ++impl_->idCounter;
    }

    int Tabs::add(std::string title, bool closable)
    {
        const int newTabId = makeId();
        impl_->tabs.push_back(
            Tab{
                .id = newTabId,
                .title = std::move(title),
                .closable = closable,
            }
        );
        Nui::WebApi::Console::log("Added tab with id: ", newTabId);
        impl_->tabs.eventContext().sync();
        return newTabId;
    }

    void Tabs::remove(int id)
    {
        auto& v = impl_->tabs.value();
        v.erase(
            std::remove_if(
                v.begin(),
                v.end(),
                [&](auto const& t)
                {
                    return t.id == id;
                }
            ),
            v.end()
        );
        impl_->tabs.modify();
        Nui::WebApi::Console::log("Removed tab with id", id);
    }

    void Tabs::onSelect(OnSelect cb)
    {
        impl_->onSelect = std::move(cb);
    }
    void Tabs::onClose(OnClose cb)
    {
        impl_->onClose = std::move(cb);
    }
    void Tabs::onReorder(OnReorder cb)
    {
        impl_->onReorder = std::move(cb);
    }

    // Helper: perform the reorder. `insertAt` is the slot index in the
    // *original* array (before any removal). Called from multiple drop sites.
    void Tabs::doReorder(int from, int insertAt)
    {
        auto& v = impl_->tabs.value();
        // After erasing `from`, all indices above it shift down by 1.
        int adjustedTo = (insertAt > from) ? insertAt - 1 : insertAt;
        if (from == adjustedTo)
            return;
        auto tab = v[from];
        v.erase(v.begin() + from);
        v.insert(v.begin() + adjustedTo, tab);
        impl_->tabs.modify();
        if (impl_->onReorder)
            impl_->onReorder(from, adjustedTo);
    }

    Nui::ElementRenderer Tabs::operator()()
    {
        using Nui::Elements::div;
        using Nui::Elements::button;
        using Nui::Elements::span;

        // Render a zero-width marker that expands to a green hairline when active.
        // `slotIndex` is the insertion position this gap represents.
        auto makeMarker = [this](long long slotIndex) -> Nui::ElementRenderer
        {
            return div{
                class_ = observe(impl_->dropInsertIndex)
                    .generate(
                        [slotIndex](int at) -> std::string
                        {
                            return at == static_cast<int>(slotIndex)
                                ? "script-nui-tab-drop-marker script-nui-tab-drop-marker--active"
                                : "script-nui-tab-drop-marker";
                        }
                    )
            }();
        };

        return div{
            class_ = "script-nui-tab-bar",

            // Hide marker when pointer leaves the bar entirely.
            "dragleave"_event =
                [this](WebApi::DragEvent e)
            {
                auto related = e.val()["relatedTarget"];
                if (related.isNull() || related.isUndefined())
                {
                    impl_->dropInsertIndex = -1;
                    Nui::globalEventContext.executeActiveEventsImmediately();
                }
            },

            "dragover"_event =
                [](WebApi::DragEvent e)
            {
                e.preventDefault();
            },

            // Bar-level drop fallback (cursor in padding/gap area).
            "drop"_event =
                [this](WebApi::DragEvent)
            {
                if (!impl_->dragSource)
                    return;

                int insertAt = impl_->dropInsertIndex.value();
                if (insertAt >= 0)
                    doReorder(*impl_->dragSource, insertAt);
                else
                {
                    // Last position:
                    doReorder(*impl_->dragSource, static_cast<int>(impl_->tabs.value().size()));
                }

                impl_->dropInsertIndex = -1;
                impl_->dragSource.reset();
            }
        }(
            // Each range item is a display:contents wrapper so both the
            // leading marker and the tab div are direct flex children of the bar.
            Nui::range(impl_->tabs)
                .after(
                    // Sentinel marker for the slot AFTER the last tab.
                    // Its slotIndex will be set dynamically by the last tab's dragover.
                    // We use a large sentinel value; the dragover handler clamps correctly.
                    // Actually we use observe so it reacts: we pass the tab count as slot.
                    // Simpler: just render one more marker whose slotIndex we observe.
                    div{class_ = observe(impl_->dropInsertIndex)
                            .generate(
                                [this](int at) -> std::string
                                {
                                    int lastSlot = static_cast<int>(impl_->tabs.value().size());
                                    return at == lastSlot
                                        ? "script-nui-tab-drop-marker script-nui-tab-drop-marker--active"
                                        : "script-nui-tab-drop-marker";
                                }
                            )}()
                ),
            [this, makeMarker](long long visualIndex, Tab const& tab) -> ElementRenderer
            {
                return div{
                    // display:contents: this wrapper is invisible to layout;
                    // its children become direct flex children of the bar.
                    style = "display: contents;"
                }(
                    // ── marker BEFORE this tab (slot == visualIndex) ──────
                    makeMarker(visualIndex),

                    // ── the tab ───────────────────────────────────────────
                    div{
                        class_ = observe(impl_->selectedId).generate(
                            [tabId = tab.id](int sel)
                            {
                                return sel == tabId
                                    ? "script-nui-tab selected"
                                    : "script-nui-tab";
                            }
                        ),
                        draggable = "true",

                        onClick = [this, tabId = tab.id](Nui::WebApi::MouseEvent event)
                        {
                            event.stopPropagation();
                            if (tabId == impl_->selectedId.value())
                                return;
                            if (impl_->onSelect)
                            {
                                if (impl_->onSelect(tabId))
                                    impl_->selectedId = tabId;
                            }
                            else
                                impl_->selectedId = tabId;
                        },

                        "dragstart"_event = [this, i = visualIndex](WebApi::DragEvent e)
                        {
                            impl_->dragSource = static_cast<int>(i);
                            e.val()["dataTransfer"].call<void>(
                                "setData", Nui::val("text/plain"), Nui::val(""));
                        },

                        "dragend"_event = [this](WebApi::DragEvent)
                        {
                            impl_->dropInsertIndex = -1;
                            impl_->dragSource.reset();
                            Nui::globalEventContext.executeActiveEventsImmediately();
                        },

                        // Read cursor position within this tab to decide which
                        // neighbouring slot boundary is closer.
                        "dragover"_event = [this, i = visualIndex](WebApi::DragEvent e)
                        {
                            e.preventDefault();
                            auto rect   = e.val()["currentTarget"]
                                            .call<Nui::val>("getBoundingClientRect");
                            double left  = rect["left"].as<double>();
                            double width = rect["width"].as<double>();
                            double cx    = e.val()["clientX"].as<double>();

                            int newInsert = (cx - left < width / 2.0)
                                ? static_cast<int>(i)
                                : static_cast<int>(i) + 1;

                            if (impl_->dropInsertIndex.value() != newInsert)
                            {
                                impl_->dropInsertIndex = newInsert;
                                Nui::globalEventContext.executeActiveEventsImmediately();
                            }
                        },

                        "drop"_event = [this](WebApi::DragEvent)
                        {
                            if (!impl_->dragSource)
                                return;
                            int insertAt = impl_->dropInsertIndex.value();
                            if (insertAt >= 0)
                                doReorder(*impl_->dragSource, insertAt);
                            impl_->dropInsertIndex = -1;
                            impl_->dragSource.reset();
                        }
                    }(
                        span{}(tab.title),
                        tab.closable
                            ? button{
                                onClick = [this, tabId = tab.id](Nui::WebApi::MouseEvent e)
                                {
                                    e.stopPropagation();
                                    if (impl_->onClose && impl_->onClose(tabId))
                                        remove(tabId);
                                }
                              }("x")
                            : Nui::ElementRenderer{Nui::nil()}
                    )
                );
            }
        );
    }
}
