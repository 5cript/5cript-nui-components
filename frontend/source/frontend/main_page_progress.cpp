#include <frontend/main_page.hpp>

#include <script-nui-components/button.hpp>
#include <script-nui-components/progress_bar.hpp>
#include <script-nui-components/progress_color.hpp>
#include <script-nui-components/progress_ring.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

using namespace ScriptNuiComponents;

namespace
{
    Nui::ElementRenderer labeledRow(std::string const& label, Nui::ElementRenderer content)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        // clang-format off
        return div{style = "display: grid; grid-template-columns: 150px 1fr; gap: 8px; align-items: center;"}(
            span{style = "opacity: 0.7;"}(label),
            std::move(content)
        );
        // clang-format on
    }

    Nui::ElementRenderer labeledItem(std::string const& label, Nui::ElementRenderer content)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        // clang-format off
        return div{style = "display: flex; flex-direction: column; align-items: center; gap: 4px;"}(
            std::move(content),
            span{style = "opacity: 0.7; font-size: 0.8em;"}(label)
        );
        // clang-format on
    }

    Nui::ElementRenderer hueSwatch(double fraction)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        return div{
            style = fmt::format(
                "width: 28px; height: 16px; border-radius: 3px; background-color: {};", progressHueColor(fraction)
            ),
            Nui::Attributes::title = fmt::format("{:.1f} %: hue {:.0f}", fraction * 100.0, progressHue(fraction)),
        }();
    }

    Nui::ElementRenderer hueSwatches()
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // clang-format off
        return div{style = "display: flex; gap: 4px;"}(
            hueSwatch(0.0),
            hueSwatch(0.125),
            hueSwatch(0.25),
            hueSwatch(0.375),
            hueSwatch(0.5),
            hueSwatch(0.625),
            hueSwatch(0.75),
            hueSwatch(0.875),
            hueSwatch(1.0)
        );
        // clang-format on
    }
} // namespace

void MainPage::stepProgress()
{
    const auto maximum = progressMaximum_->value();
    const auto current = progressCurrent_->value();
    const auto next = current >= maximum ? std::int64_t{0} : std::min(maximum, current + maximum / 8);

    *progressCurrent_ = next;
    imperativeBar_.setProgress(next);
    *ringFraction_ = maximum > 0 ? static_cast<double>(next) / static_cast<double>(maximum) : 0.0;
}

Nui::ElementRenderer MainPage::progressBarSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using ScriptNuiComponents::button;

    // clang-format off
    return div{style = "display: flex; flex-direction: column; gap: 8px;"}(
        labeledRow("Hue curve", hueSwatches()),
        labeledRow("Percent, hue", progressBar({
            .current = progressCurrent_,
            .maximum = progressMaximum_,
        })),
        labeledRow("Counts, theme", progressBar({
            .current = progressCurrent_,
            .maximum = progressMaximum_,
            .textMode = ProgressBarTextMode::Counts,
            .colorMode = ProgressColorMode::Theme,
        })),
        labeledRow("Bytes, 11px", progressBar({
            .current = progressCurrent_,
            .maximum = progressMaximum_,
            .textMode = ProgressBarTextMode::Bytes,
            .height = "11px",
        })),
        labeledRow("No text, no shine", progressBar({
            .current = progressCurrent_,
            .maximum = progressMaximum_,
            .textMode = ProgressBarTextMode::None,
            .shine = false,
        })),
        labeledRow("Custom text", progressBar({
            .current = progressCurrent_,
            .maximum = progressMaximum_,
            .formatText = [](ProgressBarValues const& values) {
                return fmt::format("{} of 8 steps", static_cast<int>(values.fraction * 8.0));
            },
        })),
        labeledRow("0 of 0", progressBar({
            .current = progressZero_,
            .maximum = progressZero_,
        })),
        labeledRow("0 of 0, zero complete", progressBar({
            .current = progressZero_,
            .maximum = progressZero_,
            .zeroAsComplete = true,
        })),
        labeledRow("Imperative", imperativeBar_()),
        div{style = "display: flex; gap: 6px; flex-wrap: wrap;"}(
            button({
                .text = "Step",
                .attributes = {onClick = [this]() { stepProgress(); }},
                .styleVariant = StyleVariant::Primary,
            }),
            button({
                .text = "Imperative: bytes / counts",
                .attributes = {onClick = [this]() {
                    imperativeBar_.setTextMode(
                        imperativeBar_.textMode() == ProgressBarTextMode::Bytes ? ProgressBarTextMode::Counts
                                                                                : ProgressBarTextMode::Bytes
                    );
                }},
            }),
            button({
                .text = "Imperative: zero as complete",
                .attributes = {onClick = [this]() {
                    imperativeBar_.setMaximum(0);
                    imperativeBar_.setProgress(0);
                    imperativeBar_.setZeroAsComplete();
                }},
            })
        )
    );
    // clang-format on
}

Nui::ElementRenderer MainPage::progressRingSection()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;
    using ScriptNuiComponents::button;

    // clang-format off
    return div{style = "display: flex; align-items: flex-end; gap: 16px; flex-wrap: wrap;"}(
        labeledItem("Hue", progressRing({.fraction = ringFraction_, .label = "Hue ring"})),
        labeledItem("Theme", progressRing({.fraction = ringFraction_, .colorMode = ProgressColorMode::Theme})),
        labeledItem("32px", progressRing({
            .fraction = ringFraction_,
            .attributes = {style = "--script-nui-components-progress-ring-size: 32px;"},
        })),
        labeledItem("Indeterminate", progressRing({.indeterminate = ringIndeterminate_})),
        button({
            .text = "Step",
            .attributes = {onClick = [this]() { stepProgress(); }},
            .styleVariant = StyleVariant::Primary,
        }),
        button({
            .text = "Toggle spinning",
            .attributes = {onClick = [this]() { ringIndeterminate_ = !ringIndeterminate_.value(); }},
        })
    );
    // clang-format on
}
