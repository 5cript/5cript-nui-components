#pragma once

#include <script-nui-components/badge.hpp>
#include <script-nui-components/style_variant.hpp>
#include <script-nui-components/state_transformers/text_node.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-tab-strip)
#    include "../../../styles/tab_strip.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /**
     * @brief A count badge after a tab title, rendered with countBadge() and hidden at 0.
     */
    struct TabStripBadge
    {
        /**
         * @brief The count, see BadgeCount.
         */
        BadgeCount count = 0;

        /**
         * @brief Selects the badge colors.
         */
        StyleVariant styleVariant = StyleVariant::Regular;
    };

    /**
     * @brief One tab of a tab strip.
     */
    struct TabStripTab
    {
        /**
         * @brief Identifies the tab in TabStripOptions::selected and onSelect, rendered as data-tab-key.
         */
        std::string key = {};

        /**
         * @brief The title (plain value, Observed or combinator).
         */
        Nui::StateTransformer<StateTransformers::TextNode> title = "";

        /**
         * @brief Optional count badge after the title.
         */
        std::optional<TabStripBadge> badge = std::nullopt;

        /**
         * @brief Extra attributes merged onto the tab button.
         */
        std::vector<Nui::Attribute> attributes = {};
    };

    /**
     * @brief Options of a tab strip.
     */
    struct TabStripOptions
    {
        /**
         * @brief The tabs in display order.
         */
        std::vector<TabStripTab> tabs = {};

        /**
         * @brief The key of the selected tab. A null pointer, or a key no tab has, selects none.
         */
        std::shared_ptr<Nui::Observed<std::string>> selected = {};

        /**
         * @brief Called with the key of a tab the user activates, after selected was assigned it.
         */
        std::function<void(std::string const& key)> onSelect = {};

        /**
         * @brief Accessible label of the tab list (aria-label). Empty sets none.
         */
        std::string label = {};

        /**
         * @brief Extra attributes merged onto the strip root.
         */
        std::vector<Nui::Attribute> attributes = {};
    };

    /**
     * @brief A row of fixed, non-draggable tabs with optional count badges; for a closable or reorderable bar see
     *        Tabs.
     *
     * Markup: div.script-nui-tab-strip[role=tablist] holding one
     * button.script-nui-tab-strip-tab[role=tab][type=button][data-tab-key][aria-selected] per tab, with
     * data-selected present on the selected one. Each tab holds span.script-nui-tab-strip-title and, when it has
     * one, the count badge (span.script-nui-badge). The selected tab has tabindex 0, the others -1; while no tab is
     * selected the first one has tabindex 0.
     *
     * Clicking a tab, or Left / Right (wrapping) and Home / End on a focused tab, selects it: the strip assigns the
     * key to selected (when it differs), moves the focus with the keys and calls onSelect. Keys pressed with Alt, Ctrl
     * or Meta are left alone. Without a selected pointer the strip keeps the selection itself. The tabs listen with
     * addEventListener, so click and keydown handlers in TabStripTab::attributes run next to their own.
     *
     * @code
     * auto selected = std::make_shared<Nui::Observed<std::string>>("conflicts");
     * tabStrip({
     *     .tabs = {
     *         {.key = "conflicts", .title = "Conflicts", .badge = TabStripBadge{conflictCount, StyleVariant::Warning}},
     *         {.key = "options", .title = "Options"},
     *     },
     *     .selected = selected,
     * });
     * @endcode
     *
     * @param options The options.
     * @return The strip.
     */
    Nui::ElementRenderer tabStrip(TabStripOptions options);

} // namespace ScriptNuiComponents
