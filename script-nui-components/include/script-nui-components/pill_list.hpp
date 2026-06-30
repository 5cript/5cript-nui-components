#pragma once

#include <script-nui-components/pill.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <memory>
#include <vector>

namespace ScriptNuiComponents
{
    /**
     * @brief A horizontally-wrapping, reactive row of pills.
     *
     * @param pills Shared, observed list of pill definitions; mutate it to grow/shrink the row.
     *              A shared_ptr is taken (rather than a reference or value) so the Observed is
     *              never moved and its lifetime is shared with the component.
     * @param attributes Extra attributes merged onto the list container.
     */
    struct PillListOptions
    {
        std::shared_ptr<Nui::Observed<std::vector<PillOptions>>> pills = {};
        std::vector<Nui::Attribute> attributes = {};
    };

    Nui::ElementRenderer pillList(PillListOptions options);

} // namespace ScriptNuiComponents
