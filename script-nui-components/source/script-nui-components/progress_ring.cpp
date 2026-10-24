#include <script-nui-components/progress_ring.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/elements/svg/circle.hpp>
#include <nui/frontend/elements/svg/svg.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ScriptNuiComponents
{
    namespace
    {
        using SharedFraction = std::shared_ptr<Nui::Observed<double>>;

        SharedFraction sharedOrZero(SharedFraction fraction)
        {
            if (fraction)
                return fraction;
            static auto const zero = std::make_shared<Nui::Observed<double>>(0.0);
            return zero;
        }

        double clampedFraction(std::weak_ptr<Nui::Observed<double>> const& fraction)
        {
            if (const auto shared = fraction.lock(); shared)
                return std::clamp(shared->value(), 0.0, 1.0);
            return 0.0;
        }
    } // namespace

    Nui::ElementRenderer progressRing(ProgressRingOptions options)
    {
        using namespace Nui::Attributes;
        namespace se = Nui::Elements::Svg;

        const auto fraction = sharedOrZero(std::move(options.fraction));
        const auto read = [fraction = std::weak_ptr<Nui::Observed<double>>{fraction}]() {
            return clampedFraction(fraction);
        };
        auto [indeterminate] = options.indeterminate.reify();

        std::vector<Nui::Attribute> rootAttributes;
        rootAttributes.reserve(10u);
        rootAttributes.push_back(class_ = "script-nui-progress-ring");
        rootAttributes.push_back("viewBox"_attr = "0 0 24 24");
        rootAttributes.push_back("role"_attr = "progressbar");
        rootAttributes.push_back("aria-valuemin"_attr = "0");
        rootAttributes.push_back("aria-valuemax"_attr = "100");
        rootAttributes.push_back("aria-valuenow"_attr = Nui::observe(fraction).generate([read]() {
            return fmt::format("{}", static_cast<int>(100.0 * read()));
        }));
        rootAttributes.push_back("data-color-mode"_attr = toString(options.colorMode));
        rootAttributes.push_back("data-complete"_attr = Nui::observe(fraction).generate([read]() {
            return read() >= 1.0;
        }));
        rootAttributes.push_back(std::move(indeterminate));
        if (!options.label.empty())
            rootAttributes.push_back("aria-label"_attr = std::move(options.label));

        std::vector<Nui::Attribute> fillAttributes;
        fillAttributes.reserve(8u);
        fillAttributes.push_back(class_ = "script-nui-progress-ring-fill");
        fillAttributes.push_back("cx"_attr = "12");
        fillAttributes.push_back("cy"_attr = "12");
        fillAttributes.push_back("r"_attr = "9");
        fillAttributes.push_back("pathLength"_attr = "100");
        fillAttributes.push_back("stroke-dashoffset"_attr = Nui::observe(fraction).generate([read]() {
            return fmt::format("{:.2f}", 100.0 - 100.0 * read());
        }));
        if (options.colorMode == ProgressColorMode::Hue)
        {
            fillAttributes.push_back("stroke"_attr = Nui::observe(fraction).generate([read]() {
                return progressHueColor(read());
            }));
        }

        // clang-format off
        return se::svg{Nui::mergeAttributes(std::move(rootAttributes), std::move(options.attributes))}(
            se::circle{
                class_ = "script-nui-progress-ring-track",
                "cx"_attr = "12",
                "cy"_attr = "12",
                "r"_attr = "9",
            }(),
            se::circle{std::move(fillAttributes)}()
        );
        // clang-format on
    }

} // namespace ScriptNuiComponents
