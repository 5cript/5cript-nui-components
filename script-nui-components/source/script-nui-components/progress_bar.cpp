#include <script-nui-components/progress_bar.hpp>

#include <nui/frontend/elements/nil.hpp>

#include <utility>

namespace ScriptNuiComponents
{
    namespace
    {
        ProgressBarTextMode textModeOf(ProgressBarSettings const& settings)
        {
            if (!settings.showMinMax)
                return ProgressBarTextMode::Percent;
            return settings.byteMode ? ProgressBarTextMode::Bytes : ProgressBarTextMode::Counts;
        }
    } // namespace

    Nui::ElementRenderer progressBar(ProgressBarOptions)
    {
        return Nui::nil();
    }

    struct ProgressBar::Implementation
    {
        ProgressBarSettings settings{};
        Nui::Observed<std::int64_t> current{0};
        Nui::Observed<std::int64_t> maximum{0};
        Nui::Observed<bool> zeroAsComplete{false};
        Nui::Observed<ProgressBarTextMode> textMode{ProgressBarTextMode::Percent};
    };

    ProgressBar::ProgressBar(Settings settings)
        : impl_{std::make_unique<Implementation>()}
    {
        impl_->current = settings.min;
        impl_->maximum = settings.max;
        impl_->textMode = textModeOf(settings);
        impl_->settings = std::move(settings);
    }

    ProgressBar::~ProgressBar() = default;
    ProgressBar::ProgressBar(ProgressBar&&) noexcept = default;
    ProgressBar& ProgressBar::operator=(ProgressBar&&) noexcept = default;

    Nui::ElementRenderer ProgressBar::operator()(std::string const&) const
    {
        return Nui::nil();
    }

    void ProgressBar::setProgress(std::int64_t current)
    {
        impl_->current = current;
    }

    std::int64_t ProgressBar::progress() const
    {
        return impl_->current.value();
    }

    void ProgressBar::setMaximum(std::int64_t maximum)
    {
        impl_->maximum = maximum;
    }

    std::int64_t ProgressBar::maximum() const
    {
        return impl_->maximum.value();
    }

    void ProgressBar::setZeroAsComplete(bool zeroAsComplete)
    {
        impl_->zeroAsComplete = zeroAsComplete;
    }

    void ProgressBar::setTextMode(ProgressBarTextMode textMode)
    {
        impl_->textMode = textMode;
    }

    ProgressBarTextMode ProgressBar::textMode() const
    {
        return impl_->textMode.value();
    }

} // namespace ScriptNuiComponents
