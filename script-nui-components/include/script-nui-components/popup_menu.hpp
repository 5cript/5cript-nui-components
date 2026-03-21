#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <functional>
#include <string>
#include <variant>
#include <vector>
#include <memory>

namespace ScriptNuiComponents
{
    /// A context-menu / dropdown popup with optional icons, separators, and
    /// section headers.  The caller is responsible for positioning by setting
    /// `x` / `y` (CSS `left` / `top` in pixels).
    ///
    /// Usage example:
    ///
    ///   PopupMenu menu;
    ///   menu.setItems({
    ///       PopupMenu::item("Open",   "📂", []{…}),
    ///       PopupMenu::item("Save",   "💾", []{…}),
    ///       PopupMenu::separator(),
    ///       PopupMenu::sectionHeader("Danger zone"),
    ///       PopupMenu::item("Delete", "🗑",  []{…}, /*disabled=*/false),
    ///   });
    ///   menu.open(mouseX, mouseY);
    ///
    ///   // In your render tree:
    ///   menu()
    class PopupMenu
    {
      public:
        // ── Item descriptor ─────────────────────────────────────────────────

        /// A clickable menu entry.
        struct MenuItem
        {
            std::string label;
            std::string icon; ///< UTF-8 icon/emoji; empty = no icon (grid still aligned)
            std::string shortcut; ///< Optional keyboard-shortcut hint shown right-aligned
            bool disabled{false};
            std::function<void()> action;
        };

        /// A horizontal rule between groups of items.
        struct Separator
        {};

        /// A non-interactive group caption.
        struct SectionHeader
        {
            std::string title;
        };

        using Entry = std::variant<MenuItem, Separator, SectionHeader>;

        // ── Convenience factories ────────────────────────────────────────────

        static Entry item(
            std::string label,
            std::string icon = {},
            std::function<void()> action = {},
            bool disabled = false,
            std::string shortcut = {}
        )
        {
            return MenuItem{
                .label = std::move(label),
                .icon = std::move(icon),
                .shortcut = std::move(shortcut),
                .disabled = disabled,
                .action = std::move(action),
            };
        }

        static Entry separator()
        {
            return Separator{};
        }
        static Entry sectionHeader(std::string title)
        {
            return SectionHeader{std::move(title)};
        }

        // ── Lifecycle ────────────────────────────────────────────────────────

        PopupMenu();
        ~PopupMenu();
        PopupMenu(PopupMenu const&) = delete;
        PopupMenu(PopupMenu&&);
        PopupMenu& operator=(PopupMenu const&) = delete;
        PopupMenu& operator=(PopupMenu&&);

        // ── API ──────────────────────────────────────────────────────────────

        void setItems(std::vector<Entry> items);

        /// Show the menu at the given viewport coordinates.
        void open(double x, double y);

        /// Show the menu optimally positioned next to a DOM element.
        ///
        /// Placement priority (first that fits inside the viewport wins):
        ///   1. Below  – left edge aligned with anchor left  (preferred)
        ///   2. Below  – right edge aligned with anchor right
        ///   3. Above  – left edge aligned with anchor left
        ///   4. Above  – right edge aligned with anchor right
        ///   5. Right  of the anchor
        ///   6. Left   of the anchor
        ///   7. Fallback: best-fit clamped to viewport with a small margin
        ///
        /// Because WebKitGTK does not support CSS Anchor Positioning the menu
        /// dimensions are measured in a hidden first-pass render (off-screen,
        /// visibility:hidden) via requestAnimationFrame, then the final position
        /// is committed in a second RAF tick so there is no visible flicker.
        ///
        /// @param anchorId  The HTML `id` attribute of the anchor element.
        ///                  Passing an id rather than a val avoids emscripten
        ///                  handle lifetime issues across the async rAF boundary.
        ///                  The element is looked up via document.getElementById
        ///                  inside the rAF callback, at which point it is still
        ///                  guaranteed to be in the DOM.
        ///
        /// Usage:
        ///   button{ id = "my-btn" }(...)   // give the anchor a stable id
        ///   menu.openNextTo("my-btn");      // pass that id here
        void openNextTo(std::string anchorId);

        /// Programmatically close the menu.
        void close();

        bool isOpen() const;

        void modifyItemByLabel(std::string const& label, std::function<void(MenuItem*)> modifier);

        // ── Render ───────────────────────────────────────────────────────────

        Nui::ElementRenderer operator()(std::vector<Nui::Attribute> additionalAttributes = {});

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}