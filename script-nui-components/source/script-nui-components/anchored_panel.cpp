#include <script-nui-components/anchored_panel.hpp>

#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/id.hpp>

#include <fmt/format.h>

namespace ScriptNuiComponents
{
    struct AnchoredPanel::Implementation
    {
        std::string id;
        Nui::ElementRenderer body;

        Nui::Observed<bool> isOpen{false};
        Nui::Observed<std::string> headerText{""};
        Nui::Observed<StyleVariant> styleVariant{StyleVariant::Regular};
    };

    AnchoredPanel::AnchoredPanel(std::string id, Nui::ElementRenderer body)
        : impl_{std::make_unique<Implementation>(Implementation{
              .id = std::move(id),
              .body = std::move(body),
          })}
    {}

    AnchoredPanel::~AnchoredPanel() = default;
    AnchoredPanel::AnchoredPanel(AnchoredPanel&&) = default;
    AnchoredPanel& AnchoredPanel::operator=(AnchoredPanel&&) = default;

    void AnchoredPanel::open(OpenOptions const& options)
    {
        impl_->headerText = options.headerText;
        impl_->styleVariant = options.styleVariant;
        impl_->isOpen = true;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    void AnchoredPanel::close()
    {
        impl_->isOpen = false;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    bool AnchoredPanel::isOpen() const
    {
        return impl_->isOpen.value();
    }

    Nui::ElementRenderer AnchoredPanel::operator()()
    {
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        return div{
            id = impl_->id,
            class_ = "script-nui-anchored-panel",
            "open"_attr = observe(impl_->isOpen).generate([this]() -> std::optional<std::string> {
                return impl_->isOpen.value() ? std::optional<std::string>{""} : std::nullopt;
            }),
        }(
            div{
                class_ = observe(impl_->styleVariant).generate([this]() {
                    return fmt::format("script-nui-anchored-panel-header {}", toString(impl_->styleVariant.value()));
                }),
            }(impl_->headerText),
            div{class_ = "script-nui-anchored-panel-body"}(impl_->body)
        );
    }
} // namespace ScriptNuiComponents
