#include <script-nui-components/pill.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>

#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/text.hpp>
#include <nui/frontend/elements/nil.hpp>

#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/on_click.hpp>

#include <ui5-sap-icons/icons/decline.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer pill(PillOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::span;

        std::vector<Nui::Attribute> baseAttributes;
        baseAttributes.reserve(4);
        baseAttributes.push_back(class_ = "script-nui-pill");
        baseAttributes.push_back("data-selected"_attr = options.selected ? "true" : "false");
        if (options.onClick)
        {
            baseAttributes.push_back("data-clickable"_attr = "true");
            baseAttributes.push_back(onClick = [cb = std::move(options.onClick)](Nui::val) {
                cb();
            });
        }

        auto [label] = options.text.reify();

        const bool removable = static_cast<bool>(options.onRemove);

        auto removeButton = removable
            ? span{
                  class_ = "script-nui-pill-remove",
                  onClick =
                      [cb = std::move(options.onRemove)](Nui::val event)
                  {
                      event.call<void>("stopPropagation");
                      cb();
                  },
              }(Ui5Icons::decline())
            : Nui::nil();

        return span{mergeAttributes(std::move(baseAttributes), std::move(options.attributes))}(
            options.icon ? *options.icon : Nui::nil(),
            span{class_ = "script-nui-pill-label"}(std::move(label)),
            std::move(removeButton)
        );
    }

} // namespace ScriptNuiComponents
