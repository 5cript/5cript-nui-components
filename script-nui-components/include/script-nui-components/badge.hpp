#pragma once

#include <script-nui-components/style_variant.hpp>
#include <script-nui-components/state_transformers/style_variant.hpp>
#include <script-nui-components/state_transformers/text_node.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/elements/text.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

#include <fmt/format.h>

#include <type_traits>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-badge)
#    include "../../../styles/badge.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace BadgeDetail
    {
        /**
         * @brief Renders a hidden state as the presence of data-hidden.
         */
        struct HiddenReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                using namespace Nui::Attributes::Literals;
                return "data-hidden"_attr = statefulObject;
            }
        };

        /**
         * @brief Renders a count (integral value, Observed or combinator) as its decimal text.
         */
        struct CountTextReify
        {
            using type = Nui::ElementRenderer;

            static type reify(Nui::StateTransformerBase const&, auto& count)
            {
                using Count = std::decay_t<decltype(count)>;
                if constexpr (Nui::IsObservedValueCombinatorWithGenerator<Count>)
                {
                    return Nui::Elements::text{count.combinator().generate([count]() {
                        return fmt::format("{}", count.value());
                    })}();
                }
                else if constexpr (Nui::IsObserved<Count>)
                {
                    return Nui::Elements::text{Nui::observe(count).generate([](auto const& value) {
                        return fmt::format("{}", value);
                    })}();
                }
                else
                    return Nui::Elements::text{fmt::format("{}", count)}();
            }
        };

        /**
         * @brief Renders a count of zero as the presence of data-hidden.
         */
        struct CountHiddenReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& count)
            {
                using namespace Nui::Attributes::Literals;
                using Count = std::decay_t<decltype(count)>;
                if constexpr (Nui::IsObservedValueCombinatorWithGenerator<Count>)
                {
                    return "data-hidden"_attr = count.combinator().generate([count]() {
                        return count.value() == 0;
                    });
                }
                else if constexpr (Nui::IsObserved<Count>)
                {
                    return "data-hidden"_attr = Nui::observe(count).generate([](auto const& value) {
                        return value == 0;
                    });
                }
                else
                    return "data-hidden"_attr = count == 0;
            }
        };
    }

    /**
     * @brief A count for countBadge(): an integral value, an Observed of one (reference or shared_ptr), or a
     *        combinator generating one.
     */
    using BadgeCount = Nui::StateTransformer<BadgeDetail::CountTextReify, BadgeDetail::CountHiddenReify>;

    /**
     * @brief Options of a badge.
     */
    struct BadgeOptions
    {
        /**
         * @brief The text (plain value, Observed or combinator). A dot badge shows none.
         */
        Nui::StateTransformer<StateTransformers::TextNode> text = "";

        /**
         * @brief Selects the colors, rendered as data-variant.
         */
        Nui::StateTransformer<StateTransformers::StyleVariantTransformer> styleVariant = StyleVariant::Regular;

        /**
         * @brief Hides the badge (plain bool, Observed or combinator), rendered as data-hidden.
         */
        Nui::StateTransformer<BadgeDetail::HiddenReify> hidden = false;

        /**
         * @brief Renders a small dot instead of the text, as data-dot.
         */
        bool dot = false;

        /**
         * @brief Non-owning; incrementing its value replays the pop animation (odd values render data-pop "b", even
         *        ones "a"). 0 or less renders no data-pop, so the badge does not pop on mount. Null disables it.
         */
        Nui::Observed<int>* pop = nullptr;

        /**
         * @brief Extra attributes merged onto the badge root.
         */
        std::vector<Nui::Attribute> attributes = {};
    };

    /**
     * @brief A small rounded label for counts and states.
     *
     * Markup contract, also produced outside Nui (for example by a dock tab renderer in TypeScript):
     * @code{.html}
     * <span class="script-nui-badge" data-variant="warning">3</span>
     * <span class="script-nui-badge" data-variant="danger" data-hidden>0</span>
     * <span class="script-nui-badge" data-variant="primary" data-dot></span>
     * @endcode
     * - class: exactly "script-nui-badge"; the stylesheet needs nothing else.
     * - data-variant: toString(StyleVariant), one of regular, primary, warning, danger, success, transparent. Missing
     *   looks like regular.
     * - data-hidden, data-dot: presence only, the value is ignored (Nui writes the attribute name as value).
     * - data-pop: optional, "a" or "b"; switching between them replays the pop animation.
     * - Content: the text as the only child. A dot badge has no text.
     *
     * @param options The options.
     * @return The badge.
     */
    Nui::ElementRenderer badge(BadgeOptions options);

    /**
     * @brief A badge showing a count, hidden while the count is 0.
     *
     * @code
     * Nui::Observed<std::size_t> conflicts{0u};
     * countBadge(conflicts, StyleVariant::Warning);
     * @endcode
     *
     * @param count The count, see BadgeCount.
     * @param styleVariant Selects the colors (plain value, Observed or combinator).
     * @param attributes Extra attributes merged onto the badge root.
     * @return The badge.
     */
    Nui::ElementRenderer countBadge(
        BadgeCount count,
        Nui::StateTransformer<StateTransformers::StyleVariantTransformer> styleVariant = StyleVariant::Regular,
        std::vector<Nui::Attribute> attributes = {}
    );

} // namespace ScriptNuiComponents
