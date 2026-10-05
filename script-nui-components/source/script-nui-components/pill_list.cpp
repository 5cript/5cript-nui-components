#include <script-nui-components/pill_list.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/event_system/range.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer pillList(PillListOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        if (!options.pills)
            return Nui::nil();

        return div{mergeAttributes(
            std::vector<Nui::Attribute>{class_ = "script-nui-pill-list"},
            std::move(options.attributes)
        )}(
            Nui::range(options.pills),
            [](long long, auto const& pillOptions) -> Nui::ElementRenderer {
                return pill(pillOptions);
            }
        );
    }

} // namespace ScriptNuiComponents
