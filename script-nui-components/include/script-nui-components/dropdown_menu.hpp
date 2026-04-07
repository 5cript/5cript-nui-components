#pragma once

#include <script-nui-components/popup_menu.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/attributes.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ScriptNuiComponents
{
    /**
     * @brief A dropdown menu consisting of a labelled trigger button with a
     * chevron indicator and an associated PopupMenu that opens below the button.
     *
     * Usage example:
     *
     *   DropdownMenu sortMenu;
     *   sortMenu.setItems({ PopupMenu::item("Name ↑", {}, []{…}), … });
     *   sortMenu.setOnOpen([&]{ otherMenu.close(); });
     *
     *   // In your render tree (pass a stable per-instance id):
     *   sortMenu("Sort", "my-sort-btn")
     *
     * The rendered element contains both the button and the popup container
     * so a single call to operator()() is all that is needed.
     */
    class DropdownMenu
    {
      public:
        DropdownMenu();
        ~DropdownMenu();
        DropdownMenu(DropdownMenu const&) = delete;
        DropdownMenu& operator=(DropdownMenu const&) = delete;
        DropdownMenu(DropdownMenu&&) noexcept;
        DropdownMenu& operator=(DropdownMenu&&) noexcept;

        /**
         * @brief Replace the current item list.
         *
         * @param items New entries to display.
         */
        void setItems(std::vector<PopupMenu::Entry> items);

        /**
         * @brief Set a callback invoked synchronously before the popup opens.
         * Use this to close sibling menus.
         *
         * @param callback Callback function to invoke on open.
         */
        void setOnOpen(std::function<void()> callback);

        /**
         * @brief Programmatically close the popup.
         */
        void close();

        /**
         * @brief Returns whether the popup is currently open.
         *
         * @return bool
         */
        bool isOpen() const;

        /**
         * @brief Render the trigger button together with the popup container.
         *
         * @param label Text shown on the trigger button.
         * @param anchorId Stable HTML id assigned to the trigger button.
         *                 Must be unique on the page; used internally by openNextTo().
         * @param additionalAttributes Extra Nui attributes merged onto the outer wrapper div.
         * @return Nui::ElementRenderer
         */
        Nui::ElementRenderer operator()(
            std::string label,
            std::string anchorId,
            std::vector<Nui::Attribute> additionalAttributes = {}
        );

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
} // namespace ScriptNuiComponents
