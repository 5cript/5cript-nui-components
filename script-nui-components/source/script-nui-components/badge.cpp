#include <script-nui-components/badge.hpp>

#include <nui/frontend/elements/nil.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer badge(BadgeOptions)
    {
        return Nui::nil();
    }

    Nui::ElementRenderer countBadge(
        BadgeCount,
        Nui::StateTransformer<StateTransformers::StyleVariantTransformer>,
        std::vector<Nui::Attribute>
    )
    {
        return Nui::nil();
    }

} // namespace ScriptNuiComponents
