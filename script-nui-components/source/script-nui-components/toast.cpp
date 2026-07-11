#include <script-nui-components/toast.hpp>
#include <script-nui-components/utf8.hpp>

#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/range.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/val.hpp>

#include <ui5-sap-icons/icons/error.hpp>
#include <ui5-sap-icons/icons/information.hpp>
#include <ui5-sap-icons/icons/sys-enter.hpp>
#include <ui5-sap-icons/icons/warning.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ScriptNuiComponents
{
    namespace
    {
        // Must match --snc-toast-leave-duration in toast.css, otherwise the toast is either removed
        // mid animation or lingers empty for a moment.
        constexpr std::int32_t leaveDurationMilliseconds = 220;

        const char* styleToString(ToastStyle style)
        {
            switch (style)
            {
                case ToastStyle::Box:
                    return "box";
                case ToastStyle::Strip:
                    return "strip";
            }
            return "box";
        }

        const char* positionToString(ToastPosition position)
        {
            switch (position)
            {
                case ToastPosition::TopLeft:
                    return "top-left";
                case ToastPosition::TopCenter:
                    return "top-center";
                case ToastPosition::TopRight:
                    return "top-right";
                case ToastPosition::BottomLeft:
                    return "bottom-left";
                case ToastPosition::BottomCenter:
                    return "bottom-center";
                case ToastPosition::BottomRight:
                    return "bottom-right";
            }
            return "bottom-right";
        }

        const char* severityToString(ToastSeverity severity)
        {
            switch (severity)
            {
                case ToastSeverity::Info:
                    return "info";
                case ToastSeverity::Success:
                    return "success";
                case ToastSeverity::Warning:
                    return "warning";
                case ToastSeverity::Error:
                    return "error";
            }
            return "info";
        }

        Nui::ElementRenderer severityIcon(ToastSeverity severity)
        {
            switch (severity)
            {
                case ToastSeverity::Info:
                    return Ui5Icons::information();
                case ToastSeverity::Success:
                    return Ui5Icons::sys_enter();
                case ToastSeverity::Warning:
                    return Ui5Icons::warning();
                case ToastSeverity::Error:
                    return Ui5Icons::error();
            }
            return Ui5Icons::information();
        }
    } // namespace

    struct Toast::Implementation
    {
        struct ActiveToast
        {
            std::uint64_t id{0};
            std::string title{};
            std::string message{};
            ToastSeverity severity{ToastSeverity::Info};
            /// Drives the leave animation; the toast is erased once it finished.
            bool leaving{false};
        };

        ToastHostOptions options{};
        Nui::Observed<ToastStyle> style{ToastStyle::Box};
        Nui::Observed<ToastPosition> position{ToastPosition::BottomRight};
        Nui::Observed<std::vector<ActiveToast>> toasts{};
        std::map<std::uint64_t, Nui::val> timers{};
        std::uint64_t nextId{1};

        std::optional<std::size_t> indexOf(std::uint64_t id) const
        {
            auto const& list = toasts.value();
            const auto found = std::ranges::find(list, id, &ActiveToast::id);
            if (found == list.end())
                return std::nullopt;
            return static_cast<std::size_t>(std::distance(list.begin(), found));
        }

        void cancelTimer(std::uint64_t id)
        {
            const auto timer = timers.find(id);
            if (timer == timers.end())
                return;
            if (!timer->second.isUndefined())
                Nui::val::global("clearTimeout")(timer->second);
            timers.erase(timer);
        }

        void schedule(std::uint64_t id, std::int32_t milliseconds, std::function<void()> what)
        {
            cancelTimer(id);
            timers[id] = Nui::val::global("setTimeout")(
                Nui::bind([this, id, what = std::move(what)]() {
                    timers.erase(id);
                    what();
                    // Timers fire outside the event context, so nothing would reach the DOM without
                    // this.
                    toasts.eventContext().sync();
                }),
                Nui::val{milliseconds}
            );
        }

        /// Plays the leave animation, then erases the toast.
        void beginLeave(std::uint64_t id)
        {
            const auto index = indexOf(id);
            if (!index)
                return;
            if (toasts.value()[*index].leaving)
                return;

            toasts[*index]->leaving = true;
            schedule(id, leaveDurationMilliseconds, [this, id]() {
                remove(id);
            });
        }

        void remove(std::uint64_t id)
        {
            const auto index = indexOf(id);
            if (!index)
                return;
            cancelTimer(id);
            toasts.erase(toasts.cbegin() + static_cast<std::ptrdiff_t>(*index));
        }

        Nui::ElementRenderer renderToast(ActiveToast const& toast);
    };

    Nui::ElementRenderer Toast::Implementation::renderToast(ActiveToast const& toast)
    {
        using Nui::Elements::button;
        using Nui::Elements::div;
        using Nui::Elements::span;
        using namespace Nui::Attributes;

        const auto id = toast.id;
        const auto dismiss = [this, id]() {
            beginLeave(id);
        };

        const auto titleElement = [&]() -> Nui::ElementRenderer {
            if (toast.title.empty())
                return Nui::nil();
            return div{class_ = "snc-toast-title"}(toast.title);
        };

        return div{
            class_ = "snc-toast",
            "data-severity"_attr = std::string{severityToString(toast.severity)},
            "data-state"_attr = std::string{toast.leaving ? "out" : "in"},
            "role"_attr = std::string{toast.severity == ToastSeverity::Error ? "alert" : "status"},
            onClick =
                [this, id]() {
                    if (options.dismissOnClick)
                        beginLeave(id);
                },
        }(
            span{class_ = "snc-toast-icon"}(severityIcon(toast.severity)),
            div{class_ = "snc-toast-content"}(titleElement(), div{class_ = "snc-toast-message"}(toast.message)),
            button{
                class_ = "snc-toast-close",
                "aria-label"_attr = std::string{"Dismiss"},
                onClick = dismiss,
            }(Utf8::cp(0x00D7))
        );
    }

    Toast::Toast(ToastHostOptions options)
        : impl_{std::make_unique<Implementation>()}
    {
        impl_->style = options.style;
        impl_->position = options.position;
        impl_->options = std::move(options);
    }

    Toast::~Toast()
    {
        if (!impl_)
            return;
        for (auto& [id, timer] : impl_->timers)
        {
            if (!timer.isUndefined())
                Nui::val::global("clearTimeout")(timer);
        }
    }

    Toast::Toast(Toast&&) = default;
    Toast& Toast::operator=(Toast&&) = default;

    Nui::ElementRenderer Toast::operator()()
    {
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        // Same as in show(): the render tree must not depend on the facade, only on the implementation.
        auto* const implementation = impl_.get();

        return div{
            class_ = "snc-toast-host",
            "data-style"_attr = observe(implementation->style)
                                    .generate([implementation]() -> std::string {
                                        return styleToString(implementation->style.value());
                                    }),
            "data-position"_attr = observe(implementation->position)
                                       .generate([implementation]() -> std::string {
                                           return positionToString(implementation->position.value());
                                       }),
        }(Nui::range(implementation->toasts), [implementation](long long, auto const& toast) -> Nui::ElementRenderer {
            return implementation->renderToast(toast);
        });
    }

    void Toast::show(ToastOptions options)
    {
        const auto id = impl_->nextId++;

        // Over the limit the oldest one leaves right away, so the new toast never has to wait for a
        // slot.
        while (impl_->toasts.value().size() >= impl_->options.maximumVisible && !impl_->toasts.value().empty())
            impl_->remove(impl_->toasts.value().front().id);

        impl_->toasts.push_back(Implementation::ActiveToast{
            .id = id,
            .title = std::move(options.title),
            .message = std::move(options.message),
            .severity = options.severity,
            .leaving = false,
        });
        impl_->toasts.eventContext().sync();

        // The timer callback outlives the Toast facade if it is moved, so it holds the implementation
        // (which stays put behind the unique_ptr), never the facade.
        const auto duration = options.durationMilliseconds.value_or(impl_->options.defaultDurationMilliseconds);
        if (duration > 0)
        {
            auto* const implementation = impl_.get();
            implementation->schedule(id, duration, [implementation, id]() {
                implementation->beginLeave(id);
            });
        }
    }

    void Toast::info(std::string message)
    {
        show({.message = std::move(message), .severity = ToastSeverity::Info});
    }

    void Toast::success(std::string message)
    {
        show({.message = std::move(message), .severity = ToastSeverity::Success});
    }

    void Toast::warning(std::string message)
    {
        show({.message = std::move(message), .severity = ToastSeverity::Warning});
    }

    void Toast::error(std::string message)
    {
        show({.message = std::move(message), .severity = ToastSeverity::Error});
    }

    void Toast::dismissAll()
    {
        for (auto& [id, timer] : impl_->timers)
        {
            if (!timer.isUndefined())
                Nui::val::global("clearTimeout")(timer);
        }
        impl_->timers.clear();

        // Everything goes at once, a full redraw of an empty list is exactly right here.
        impl_->toasts.clear();
        impl_->toasts.eventContext().sync();
    }

    void Toast::setStyle(ToastStyle style)
    {
        impl_->options.style = style;
        impl_->style = style;
        impl_->style.eventContext().sync();
    }

    void Toast::setPosition(ToastPosition position)
    {
        impl_->options.position = position;
        impl_->position = position;
        impl_->position.eventContext().sync();
    }
} // namespace ScriptNuiComponents
