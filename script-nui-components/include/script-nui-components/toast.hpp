#pragma once

#include <nui/frontend/element_renderer.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-toast)
#    include "../../../styles/toast.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /**
     * @brief Severity of a toast; selects the accent color and the icon.
     */
    enum class ToastSeverity
    {
        Info,
        Success,
        Warning,
        Error,
    };

    /**
     * @brief Visual style of the toast host.
     *
     * Box: a rounded card anchored in a corner, sliding in from the nearest edge.
     * Strip: a bar spanning the whole viewport width, moving in from above or below.
     */
    enum class ToastStyle
    {
        Box,
        Strip,
    };

    /**
     * @brief Where the toasts appear.
     *
     * The Strip style only honours the top / bottom part of the position, the horizontal part is
     * ignored because a strip always spans the full width.
     */
    enum class ToastPosition
    {
        TopLeft,
        TopCenter,
        TopRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    };

    /**
     * @brief Host configuration; applies to every toast shown by this host.
     */
    struct ToastHostOptions
    {
        /**
         * @brief Box or Strip.
         */
        ToastStyle style{ToastStyle::Box};

        /**
         * @brief Corner (Box) or edge (Strip) the toasts appear at.
         */
        ToastPosition position{ToastPosition::BottomRight};

        /**
         * @brief How long a toast stays before it leaves again, unless the toast overrides it.
         * This duration has to be in sync with the CSS.
         */
        std::int32_t defaultDurationMilliseconds{5000};

        /**
         * @brief How many toasts are on screen at once; the oldest leaves when the limit is hit.
         */
        std::size_t maximumVisible{4};

        /**
         * @brief Whether a click anywhere on the toast dismisses it, not just the close button.
         */
        bool dismissOnClick{true};
    };

    /**
     * @brief A single toast.
     */
    struct ToastOptions
    {
        /**
         * @brief The message; the only mandatory field.
         */
        std::string message{};

        /**
         * @brief Optional bold line above the message.
         */
        std::string title{};

        ToastSeverity severity{ToastSeverity::Info};

        /**
         * @brief Overrides the host default. Zero keeps the toast until it is dismissed by hand.
         */
        std::optional<std::int32_t> durationMilliseconds{std::nullopt};
    };

    /**
     * @brief Host for transient messages: each toast pops in, dwells for its duration and then
     *        moves back out of the viewport.
     *
     * Mount the host once, anywhere in the tree (it positions itself fixed to the viewport), then
     * call show() from wherever a message has to reach the user. Toasts stack in the order they
     * arrive and remove themselves when their timer expires, when they are clicked, or when the
     * host runs out of visible slots.
     *
     * @code
     * ScriptNuiComponents::Toast toasts_{{.style = ToastStyle::Box, .position = ToastPosition::BottomRight}};
     * // in the render tree:
     * toasts_()
     * // from an error path:
     * toasts_.show({.message = "Snippet was not saved", .severity = ToastSeverity::Error});
     * @endcode
     */
    class Toast
    {
      public:
        explicit Toast(ToastHostOptions options = {});
        ~Toast();
        Toast(Toast const&) = delete;
        Toast(Toast&&);
        Toast& operator=(Toast const&) = delete;
        Toast& operator=(Toast&&);

        /**
         * @brief Mounts the host. Call once, in the render tree.
         */
        Nui::ElementRenderer operator()();

        /**
         * @brief Shows a toast.
         */
        void show(ToastOptions options);

        /**
         * @brief Shortcut for show() with the matching severity.
         */
        void info(std::string message);
        void success(std::string message);
        void warning(std::string message);
        void error(std::string message);

        /**
         * @brief Removes every toast without playing the leave animation.
         */
        void dismissAll();

        /**
         * @brief Switches the style of the host; visible toasts adopt it immediately.
         */
        void setStyle(ToastStyle style);

        /**
         * @brief Switches the position of the host; visible toasts adopt it immediately.
         */
        void setPosition(ToastPosition position);

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
} // namespace ScriptNuiComponents
