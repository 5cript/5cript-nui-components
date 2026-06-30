#include <script-nui-components/collapsible_section.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>

#include <nui/frontend/elements/details.hpp>
#include <nui/frontend/elements/summary.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/text.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/svg_elements.hpp>

#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/svg_attributes.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer collapsibleSection(CollapsibleSectionOptions options, Nui::ElementRenderer content)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        namespace se = Nui::Elements::Svg;
        namespace sa = Nui::Attributes::Svg;

        std::vector<Nui::Attribute> detailsAttributes;
        detailsAttributes.reserve(3);
        detailsAttributes.push_back(class_ = "script-nui-collapsible");
        // `open` is presence-based, so only attach it when expanded.
        if (options.initiallyExpanded)
            detailsAttributes.push_back("open"_attr = "true");
        if (options.onToggle)
        {
            detailsAttributes.push_back("toggle"_event = [cb = std::move(options.onToggle)](Nui::val event) {
                cb(event["target"]["open"].as<bool>());
            });
        }

        auto [title] = options.title.reify();

        return details{mergeAttributes(std::move(detailsAttributes), std::move(options.attributes))}(
            summary{class_ = "script-nui-collapsible-summary"}(
                se::svg{
                    class_ = "script-nui-collapsible-chevron",
                    sa::viewBox = "0 0 16 16",
                }(se::path{
                    sa::d = "M6 3 L11 8 L6 13",
                    "fill"_attr = "none",
                    "stroke"_attr = "currentColor",
                    "stroke-width"_attr = "2",
                    "stroke-linecap"_attr = "round",
                    "stroke-linejoin"_attr = "round",
                }()),
                span{class_ = "script-nui-collapsible-title"}(std::move(title)),
                options.badge
                    ? span{class_ = "script-nui-collapsible-badge"}(text{*options.badge}())
                    : Nui::nil()
            ),
            div{class_ = "script-nui-collapsible-body"}(std::move(content))
        );
    }

} // namespace ScriptNuiComponents
