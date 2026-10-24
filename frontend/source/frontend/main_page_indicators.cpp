#include <frontend/main_page.hpp>

#include <script-nui-components/badge.hpp>
#include <script-nui-components/button.hpp>
#include <script-nui-components/collapsible_section.hpp>
#include <script-nui-components/list_row.hpp>
#include <script-nui-components/message_strip.hpp>
#include <script-nui-components/progress_ring.hpp>
#include <script-nui-components/tab_strip.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <ui5-sap-icons/icons/delete.hpp>
#include <ui5-sap-icons/icons/document.hpp>
#include <ui5-sap-icons/icons/status-inactive.hpp>
#include <ui5-sap-icons/icons/warning.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <string>

using namespace ScriptNuiComponents;

namespace
{
    Nui::ElementRenderer trashButton(std::string what)
    {
        using namespace Nui::Attributes;

        // clang-format off
        return ScriptNuiComponents::button({
            .icon = Ui5Icons::delete_(),
            .attributes = {
                title = "Delete",
                onClick = [what = std::move(what)]() { Nui::WebApi::Console::log("Trash clicked: ", what); },
            },
            .styleVariant = StyleVariant::Transparent,
        });
        // clang-format on
    }
} // namespace

Nui::ElementRenderer MainPage::badgeSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using Nui::Elements::span;
    using ScriptNuiComponents::button;

    // clang-format off
    return div{style = "display: flex; flex-direction: column; gap: 10px;"}(
        div{style = "display: flex; gap: 6px; align-items: center; flex-wrap: wrap;"}(
            badge({.text = "Regular"}),
            badge({.text = "Primary", .styleVariant = StyleVariant::Primary}),
            badge({.text = "Warning", .styleVariant = StyleVariant::Warning}),
            badge({.text = "Danger", .styleVariant = StyleVariant::Danger}),
            badge({.text = "Success", .styleVariant = StyleVariant::Success}),
            badge({.text = "Transparent", .styleVariant = StyleVariant::Transparent}),
            badge({.styleVariant = StyleVariant::Warning, .dot = true}),
            badge({.styleVariant = StyleVariant::Danger, .dot = true}),
            badge({.text = "Hidden", .hidden = true})
        ),
        div{style = "display: flex; gap: 6px; align-items: center; flex-wrap: wrap;"}(
            span{style = "opacity: 0.7;"}("Count, hidden at 0:"),
            countBadge(badgeCount_, StyleVariant::Warning),
            countBadge(badgeCount_, StyleVariant::Danger),
            span{style = "opacity: 0.7;"}("Pops:"),
            badge({
                .text = Nui::observe(badgeCount_).generate([this]() {
                    return fmt::format("{} new", badgeCount_.value());
                }),
                .styleVariant = StyleVariant::Primary,
                .pop = &badgePop_,
            }),
            button({
                .text = "+1",
                .attributes = {onClick = [this]() {
                    badgeCount_ = badgeCount_.value() + 1;
                    badgePop_ = badgePop_.value() + 1;
                }},
                .styleVariant = StyleVariant::Primary,
            }),
            button({
                .text = "-1",
                .attributes = {onClick = [this]() { badgeCount_ = std::max(0, badgeCount_.value() - 1); }},
            }),
            button({
                .text = "Pop",
                .attributes = {onClick = [this]() { badgePop_ = badgePop_.value() + 1; }},
            })
        )
    );
    // clang-format on
}

Nui::ElementRenderer MainPage::tabStripSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using Nui::Elements::span;

    // clang-format off
    return div{style = "display: flex; flex-direction: column; gap: 8px; max-width: 480px;"}(
        tabStrip({
            .tabs = {
                {
                    .key = "conflicts",
                    .title = "Conflicts",
                    .badge = TabStripBadge{.count = badgeCount_, .styleVariant = StyleVariant::Warning},
                },
                {
                    .key = "failed",
                    .title = "Failed",
                    .badge = TabStripBadge{.count = 2, .styleVariant = StyleVariant::Danger},
                },
                {
                    .key = "options",
                    .title = "Options",
                },
            },
            .selected = selectedTab_,
            .onSelect = [](std::string const& key) { Nui::WebApi::Console::log("Tab selected: ", key); },
            .label = "Watch details",
        }),
        div{}(
            span{style = "opacity: 0.7;"}("Selected: "),
            text{selectedTab_}()
        )
    );
    // clang-format on
}

Nui::ElementRenderer MainPage::listRowSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    const auto isSelected = [this](int row) {
        return Nui::observe(selectedRow_).generate([this, row]() { return selectedRow_.value() == row; });
    };
    const auto select = [this](int row) {
        return [this, row]() {
            selectedRow_ = row;
        };
    };

    // clang-format off
    return div{
        "role"_attr = "listbox",
        "aria-label"_attr = "Watches",
        style = "display: flex; flex-direction: column; gap: 2px; max-width: 360px;",
    }(
        listRow({
            .leading = progressRing({.fraction = ringFraction_}),
            .label = "report-with-a-rather-long-name-that-gets-ellipsized.docx",
            .sideText = Nui::observe(*ringFraction_).generate([fraction = ringFraction_]() {
                return fmt::format("{:.0f} %", fraction->value() * 100.0);
            }),
            .selected = isSelected(0),
            .onActivate = select(0),
        }),
        listRow({
            .leading = Ui5Icons::warning(),
            .label = "notes.txt",
            .sideText = "2 conflicts",
            .selected = isSelected(1),
            .onActivate = select(1),
        }),
        listRow({
            .leading = Ui5Icons::status_inactive(),
            .label = "build.log",
            .sideText = "stopped",
            .trailing = trashButton("build.log"),
            .selected = isSelected(2),
            .onActivate = select(2),
        }),
        listRow({
            .leading = Ui5Icons::document(),
            .label = "No side text",
            .selected = isSelected(3),
            .onActivate = select(3),
        })
    );
    // clang-format on
}

Nui::ElementRenderer MainPage::slotExtensionsSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using ScriptNuiComponents::button;

    // clang-format off
    return div{style = "display: flex; flex-direction: column; gap: 10px; max-width: 480px;"}(
        collapsibleSection(
            {
                .title = "Needs you",
                .headerEnd = countBadge(badgeCount_, StyleVariant::Warning),
            },
            div{}("Clicking the badge does not fold the section.")
        ),
        collapsibleSection(
            {
                .title = "Stopped",
                .headerEnd = div{style = "display: inline-flex; align-items: center; gap: 4px;"}(
                    countBadge(2),
                    trashButton("all stopped")
                ),
                .initiallyExpanded = false,
            },
            div{}("The trash button acts without folding the section.")
        ),
        messageStrip({
            .text = "Stopped: another watch covers this folder.",
            .styleVariant = StyleVariant::Warning,
            .actions = button({
                .text = "Show it",
                .attributes = {onClick = []() { Nui::WebApi::Console::log("Show it clicked"); }},
                .styleVariant = StyleVariant::Transparent,
            }),
        }),
        messageStrip({
            .text = "The local copy is gone.",
            .styleVariant = StyleVariant::Danger,
            .actions = div{style = "display: inline-flex; gap: 6px;"}(
                button({.text = "Open Again", .styleVariant = StyleVariant::Primary}),
                button({.text = "Remove", .styleVariant = StyleVariant::Transparent})
            ),
        })
    );
    // clang-format on
}
