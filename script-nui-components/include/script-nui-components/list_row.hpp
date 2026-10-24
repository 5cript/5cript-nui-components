#pragma once

#include <script-nui-components/state_transformers/text_node.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-list-row)
#    include "../../../styles/list_row.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace ListRowDetail
    {
        /**
         * @brief Renders the selected state as the presence of data-selected.
         */
        struct SelectedReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                using namespace Nui::Attributes::Literals;
                return "data-selected"_attr = statefulObject;
            }
        };

        /**
         * @brief Renders the selected state as aria-selected "true" or "false".
         */
        struct AriaSelectedReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& selected)
            {
                using namespace Nui::Attributes::Literals;
                using Selected = std::decay_t<decltype(selected)>;
                const auto toText = [](bool value) -> std::string {
                    return value ? "true" : "false";
                };
                if constexpr (Nui::IsObservedValueCombinatorWithGenerator<Selected>)
                {
                    return "aria-selected"_attr = selected.combinator().generate([selected, toText]() {
                        return toText(static_cast<bool>(selected.value()));
                    });
                }
                else if constexpr (Nui::IsObserved<Selected>)
                {
                    return "aria-selected"_attr = Nui::observe(selected).generate([toText](auto const& value) {
                        return toText(static_cast<bool>(value));
                    });
                }
                else
                    return "aria-selected"_attr = toText(static_cast<bool>(selected));
            }
        };
    }

    /**
     * @brief Options of a list row.
     */
    struct ListRowOptions
    {
        /**
         * @brief Optional icon, glyph or progress ring before the label.
         */
        std::optional<Nui::ElementRenderer> leading = std::nullopt;

        /**
         * @brief The label (plain value, Observed or combinator), ellipsized when too long.
         */
        Nui::StateTransformer<StateTransformers::TextNode> label = "";

        /**
         * @brief Muted text after the label (plain value, Observed or combinator); the part collapses while empty.
         */
        Nui::StateTransformer<StateTransformers::TextNode> sideText = "";

        /**
         * @brief Optional controls at the end, for example an icon button. Their clicks and keys never activate
         *        the row.
         */
        std::optional<Nui::ElementRenderer> trailing = std::nullopt;

        /**
         * @brief The selected state (plain bool, Observed or combinator), rendered as data-selected and
         *        aria-selected.
         */
        Nui::StateTransformer<ListRowDetail::SelectedReify, ListRowDetail::AriaSelectedReify> selected = false;

        /**
         * @brief Called on a click on the row and on Enter or Space while it has the focus.
         */
        std::function<void()> onActivate = {};

        /**
         * @brief Extra attributes merged onto the row root.
         */
        std::vector<Nui::Attribute> attributes = {};
    };

    /**
     * @brief A selectable row for lists and outliners. Place rows in an element with role=listbox.
     *
     * Markup: div.script-nui-list-row[role=option][tabindex=0][aria-selected] with data-selected present while
     * selected, holding span.script-nui-list-row-leading (only with leading), span.script-nui-list-row-label,
     * span.script-nui-list-row-side and span.script-nui-list-row-trailing (only with trailing). Clicks and keydowns
     * inside the trailing part stop propagating, so they never reach the row.
     *
     * @code
     * listRow({
     *     .label = "notes.txt",
     *     .sideText = "3 changes",
     *     .selected = isSelected,
     *     .onActivate = [this]() { select(id); },
     * });
     * @endcode
     *
     * @param options The options.
     * @return The row.
     */
    Nui::ElementRenderer listRow(ListRowOptions options);

} // namespace ScriptNuiComponents
