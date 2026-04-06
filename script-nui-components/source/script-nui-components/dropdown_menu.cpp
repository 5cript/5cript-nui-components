#include <script-nui-components/dropdown_menu.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/event_system/event_context.hpp>

#include <ui5-sap-icons/icons/navigation-down-arrow.hpp>

using namespace Nui::Elements;
using namespace Nui::Attributes;

namespace ScriptNuiComponents
{
    struct DropdownMenu::Implementation
    {
        PopupMenu popup{};
        std::function<void()> onOpen{};
    };

    DropdownMenu::DropdownMenu()
        : impl_{std::make_unique<Implementation>()}
    {}
    DropdownMenu::~DropdownMenu() = default;
    DropdownMenu::DropdownMenu(DropdownMenu&&) noexcept = default;
    DropdownMenu& DropdownMenu::operator=(DropdownMenu&&) noexcept = default;

    void DropdownMenu::setItems(std::vector<PopupMenu::Entry> items)
    {
        impl_->popup.setItems(std::move(items));
    }

    void DropdownMenu::setOnOpen(std::function<void()> callback)
    {
        impl_->onOpen = std::move(callback);
    }

    void DropdownMenu::close()
    {
        impl_->popup.close();
    }

    bool DropdownMenu::isOpen() const
    {
        return impl_->popup.isOpen();
    }

    Nui::ElementRenderer
    DropdownMenu::operator()(std::string label, std::string anchorId, std::vector<Nui::Attribute> additionalAttributes)
    {
        using Nui::Elements::div;
        using Nui::Elements::span;

        auto* impl = impl_.get();

        const auto attributes = Nui::mergeAttributes(
            std::move(additionalAttributes),
            {id = anchorId,
                class_ = "script-nui-button script-nui-dropdown-menu",
                type = "button",
                onClick = [impl, anchorId](Nui::val event)
                {
                    event.call<void>("stopPropagation");
                    if (impl->popup.isOpen())
                    {
                        impl->popup.close();
                        return;
                    }
                    if (impl->onOpen)
                        impl->onOpen();
                    impl->popup.openNextTo(anchorId);
                }}
        );

        return button{std::move(attributes)}(span{}(label), Ui5Icons::navigation_down_arrow(), impl->popup());
    }
} // namespace ScriptNuiComponents
