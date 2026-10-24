#include <script-nui-components/progress_bar.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/text.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>

namespace ScriptNuiComponents
{
    namespace
    {
        using SharedCount = std::shared_ptr<Nui::Observed<std::int64_t>>;

        /**
         * @brief The changing inputs of a bar.
         */
        struct BarState
        {
            std::int64_t current = 0;
            std::int64_t maximum = 0;
            bool zeroAsComplete = false;
            ProgressBarTextMode textMode = ProgressBarTextMode::Percent;
        };

        /**
         * @brief The fixed inputs of a bar, shared by the generators of one render.
         */
        struct BarLook
        {
            std::int64_t minimum = 0;
            ProgressColorMode colorMode = ProgressColorMode::Hue;
            bool shine = true;
            std::function<std::string(ProgressBarValues const&)> formatText = {};
            ProgressBarTexts texts = {};
        };

        constexpr std::array<std::string_view, 5> byteUnits{"B", "KB", "MB", "GB", "TB"};

        ProgressBarTextMode textModeOf(ProgressBarSettings const& settings)
        {
            if (!settings.showMinimumAndMaximum)
                return ProgressBarTextMode::Percent;
            return settings.byteMode ? ProgressBarTextMode::Bytes : ProgressBarTextMode::Counts;
        }

        std::size_t byteMagnitudeOf(std::int64_t value)
        {
            if (value < 1'000)
                return 0u;
            if (value < 1'000'000)
                return 1u;
            if (value < 1'000'000'000)
                return 2u;
            if (value < 1'000'000'000'000)
                return 3u;
            return 4u;
        }

        std::string formatBytes(std::int64_t value, std::size_t magnitude)
        {
            if (magnitude == 0u)
                return fmt::format("{}  B", value);
            return fmt::format(
                "{:.2f} {}",
                static_cast<double>(value) / std::pow(1024.0, static_cast<double>(magnitude)),
                byteUnits[magnitude]
            );
        }

        double fractionBetween(std::int64_t current, std::int64_t minimum, std::int64_t maximum, bool zeroAsComplete)
        {
            const auto lower = std::min(minimum, maximum);
            const auto upper = std::max(minimum, maximum);
            if (upper == 0)
                return zeroAsComplete ? 1.0 : 0.0;
            if (lower == upper)
                return 1.0;
            const auto clamped = std::clamp(current, lower, upper);
            return static_cast<double>(clamped - lower) / static_cast<double>(upper - lower);
        }

        ProgressBarValues valuesOf(BarState const& state, std::int64_t minimum)
        {
            return ProgressBarValues{
                .current = state.current,
                .minimum = minimum,
                .maximum = state.maximum,
                .fraction = fractionBetween(state.current, minimum, state.maximum, state.zeroAsComplete),
                .complete = state.current >= state.maximum,
            };
        }

        std::string textOf(BarLook const& look, ProgressBarTextMode textMode, ProgressBarValues const& values)
        {
            if (textMode == ProgressBarTextMode::None)
                return {};
            if (look.formatText)
                return look.formatText(values);

            const auto percent = 100.0 * values.fraction;
            switch (textMode)
            {
                case ProgressBarTextMode::Percent:
                    return fmt::format(fmt::runtime(look.texts.percent), percent);
                case ProgressBarTextMode::Counts:
                    return fmt::format(fmt::runtime(look.texts.counts), values.current, values.maximum, percent);
                case ProgressBarTextMode::Bytes:
                {
                    const auto magnitude = byteMagnitudeOf(values.maximum);
                    return fmt::format(
                        fmt::runtime(look.texts.bytes),
                        formatBytes(values.current, magnitude),
                        formatBytes(values.maximum, magnitude),
                        percent
                    );
                }
                case ProgressBarTextMode::None:
                    break;
            }
            return {};
        }

        std::string fillStyleOf(ProgressColorMode colorMode, ProgressBarValues const& values)
        {
            const auto width = fmt::format("width: {:.2f}%;", 100.0 * values.fraction);
            if (colorMode == ProgressColorMode::Theme)
                return width;
            return fmt::format(
                "{} background-color: {};", width, progressHueColor(values.complete ? 1.0 : values.fraction)
            );
        }

        SharedCount sharedOrZero(SharedCount observed)
        {
            if (observed)
                return observed;
            static auto const zero = std::make_shared<Nui::Observed<std::int64_t>>(0);
            return zero;
        }

        std::int64_t valueOf(std::weak_ptr<Nui::Observed<std::int64_t>> const& observed)
        {
            if (const auto shared = observed.lock(); shared)
                return shared->value();
            return 0;
        }

        /**
         * @brief Renders the bar markup.
         *
         * @param look The fixed inputs.
         * @param combinator Observes everything read() reads.
         * @param read Returns the current changing inputs.
         * @param rootStyle The inline style of the root, if any.
         * @param attributes Caller attributes merged onto the root.
         */
        template <typename CombinatorT, typename ReadT>
        Nui::ElementRenderer renderBar(
            std::shared_ptr<BarLook const> const& look,
            CombinatorT const& combinator,
            ReadT const& read,
            std::optional<std::string> rootStyle,
            std::vector<Nui::Attribute> attributes
        )
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            const auto currentValues = [look, read]() {
                return valuesOf(read(), look->minimum);
            };
            const auto currentText = [look, read]() {
                const auto state = read();
                return textOf(*look, state.textMode, valuesOf(state, look->minimum));
            };

            std::vector<Nui::Attribute> rootAttributes{
                class_ = "script-nui-progress-bar",
                "role"_attr = "progressbar",
                "aria-valuemin"_attr = "0",
                "aria-valuemax"_attr = "100",
                "aria-valuenow"_attr = combinator.generate([currentValues]() {
                    return fmt::format("{}", static_cast<int>(100.0 * currentValues().fraction));
                }),
                "aria-valuetext"_attr = combinator.generate(currentText),
                "data-color-mode"_attr = toString(look->colorMode),
                "data-complete"_attr = combinator.generate([currentValues]() {
                    return currentValues().complete;
                }),
            };
            if (rootStyle)
                rootAttributes.push_back(style = std::move(*rootStyle));

            // clang-format off
            return div{Nui::mergeAttributes(std::move(rootAttributes), std::move(attributes))}(
                div{
                    class_ = "script-nui-progress-bar-fill",
                    style = combinator.generate([look, currentValues]() {
                        return fillStyleOf(look->colorMode, currentValues());
                    }),
                }(),
                div{class_ = "script-nui-progress-bar-text"}(
                    text{combinator.generate(currentText)}()
                ),
                look->shine ? div{class_ = "script-nui-progress-bar-shine"}() : Nui::nil()
            );
            // clang-format on
        }
    } // namespace

    Nui::ElementRenderer progressBar(ProgressBarOptions options)
    {
        const auto current = sharedOrZero(std::move(options.current));
        const auto maximum = sharedOrZero(std::move(options.maximum));
        const auto look = std::make_shared<BarLook const>(BarLook{
            .minimum = options.minimum,
            .colorMode = options.colorMode,
            .shine = options.shine,
            .formatText = std::move(options.formatText),
            .texts = std::move(options.texts),
        });
        const auto read = [current = std::weak_ptr<Nui::Observed<std::int64_t>>{current},
                           maximum = std::weak_ptr<Nui::Observed<std::int64_t>>{maximum},
                           zeroAsComplete = options.zeroAsComplete,
                           textMode = options.textMode]() {
            return BarState{
                .current = valueOf(current),
                .maximum = valueOf(maximum),
                .zeroAsComplete = zeroAsComplete,
                .textMode = textMode,
            };
        };
        auto rootStyle = options.height ? std::optional<std::string>{fmt::format("height: {};", *options.height)}
                                        : std::nullopt;
        return renderBar(look, Nui::observe(current, maximum), read, std::move(rootStyle), std::move(options.attributes));
    }

    struct ProgressBar::Implementation
    {
        explicit Implementation(ProgressBarSettings settings)
            : look{std::make_shared<BarLook const>(BarLook{
                  .minimum = settings.minimum,
                  .colorMode = settings.colorMode,
                  .shine = settings.shine,
                  .texts = std::move(settings.texts),
              })}
            , height{std::move(settings.height)}
            , attributes{std::move(settings.attributes)}
            , current{settings.minimum}
            , maximum{settings.maximum}
            , textMode{textModeOf(settings)}
        {}

        std::shared_ptr<BarLook const> look;
        std::string height;
        std::vector<Nui::Attribute> attributes;
        Nui::Observed<std::int64_t> current;
        Nui::Observed<std::int64_t> maximum;
        Nui::Observed<bool> zeroAsComplete{false};
        Nui::Observed<ProgressBarTextMode> textMode;
    };

    ProgressBar::ProgressBar(Settings settings)
        : impl_{std::make_unique<Implementation>(std::move(settings))}
    {}

    ProgressBar::~ProgressBar() = default;
    ProgressBar::ProgressBar(ProgressBar&&) noexcept = default;
    ProgressBar& ProgressBar::operator=(ProgressBar&&) noexcept = default;

    Nui::ElementRenderer ProgressBar::operator()(std::string const& extraStyleOptions) const
    {
        Implementation const* implementation = impl_.get();
        const auto read = [implementation]() {
            return BarState{
                .current = implementation->current.value(),
                .maximum = implementation->maximum.value(),
                .zeroAsComplete = implementation->zeroAsComplete.value(),
                .textMode = implementation->textMode.value(),
            };
        };
        return renderBar(
            implementation->look,
            Nui::observe(
                implementation->current,
                implementation->maximum,
                implementation->zeroAsComplete,
                implementation->textMode
            ),
            read,
            fmt::format("height: {}; {}", implementation->height, extraStyleOptions),
            implementation->attributes
        );
    }

    void ProgressBar::setProgress(std::int64_t current)
    {
        if (impl_->current.value() != current)
            impl_->current = current;
    }

    std::int64_t ProgressBar::progress() const
    {
        return impl_->current.value();
    }

    void ProgressBar::setMaximum(std::int64_t maximum)
    {
        if (impl_->maximum.value() != maximum)
            impl_->maximum = maximum;
    }

    std::int64_t ProgressBar::maximum() const
    {
        return impl_->maximum.value();
    }

    void ProgressBar::setZeroAsComplete(bool zeroAsComplete)
    {
        if (impl_->zeroAsComplete.value() != zeroAsComplete)
            impl_->zeroAsComplete = zeroAsComplete;
    }

    void ProgressBar::setTextMode(ProgressBarTextMode textMode)
    {
        if (impl_->textMode.value() != textMode)
            impl_->textMode = textMode;
    }

    ProgressBarTextMode ProgressBar::textMode() const
    {
        return impl_->textMode.value();
    }

} // namespace ScriptNuiComponents
