#pragma once

#include <script-nui-components/progress_color.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>
#include <nui/event_system/observed_value.hpp>

#include <memory>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-progress-ring)
#    include "../../../styles/progress_ring.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace ProgressRingDetail
    {
        /**
         * @brief Renders the indeterminate state as the presence of data-indeterminate.
         */
        struct IndeterminateReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                using namespace Nui::Attributes::Literals;
                return "data-indeterminate"_attr = statefulObject;
            }
        };
    }

    /**
     * @brief Options of the progress ring.
     */
    struct ProgressRingOptions
    {
        /**
         * @brief Progress from 0 to 1, clamped. A null pointer counts as 0.
         */
        std::shared_ptr<Nui::Observed<double>> fraction = {};

        /**
         * @brief Hue colors the fill by progress (progressHueColor()); Theme uses
         *        --script-nui-components-progress-ring-theme.
         */
        ProgressColorMode colorMode = ProgressColorMode::Hue;

        /**
         * @brief Spins a fixed arc in the theme color instead of showing the fraction (plain bool, Observed or
         *        combinator).
         */
        Nui::StateTransformer<ProgressRingDetail::IndeterminateReify> indeterminate = false;

        /**
         * @brief Accessible label (aria-label). Empty sets none.
         */
        std::string label = {};

        /**
         * @brief Extra attributes merged onto the svg root.
         */
        std::vector<Nui::Attribute> attributes = {};
    };

    /**
     * @brief A small circular progress indicator, for list rows and headers.
     *
     * Markup: svg.script-nui-progress-ring[role=progressbar][data-color-mode][data-complete][data-indeterminate],
     * viewBox "0 0 24 24", holding circle.script-nui-progress-ring-track and circle.script-nui-progress-ring-fill
     * (r 9, pathLength 100, stroke-dasharray 100). The fill's stroke-dashoffset (100 - 100 * fraction) and, in Hue
     * mode, its stroke follow the fraction as attributes. data-color-mode is "hue" or "theme"; data-complete is present
     * at a fraction of 1; data-indeterminate while indeterminate. aria-valuenow carries the percentage, aria-valuemin
     * and aria-valuemax are 0 and 100.
     *
     * @code
     * auto fraction = std::make_shared<Nui::Observed<double>>(0.25);
     * progressRing({.fraction = fraction, .label = "Uploading"});
     * @endcode
     *
     * @param options The options.
     * @return The ring.
     */
    Nui::ElementRenderer progressRing(ProgressRingOptions options);

} // namespace ScriptNuiComponents
