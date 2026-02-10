#pragma once

#include <nui/frontend/attributes/impl/attribute.hpp>

#include <vector>
#include <iterator>

namespace ScriptNuiComponents
{
    inline std::vector<Nui::Attribute>
    mergeAttributes(std::vector<Nui::Attribute>&& base, std::vector<Nui::Attribute>&& additional)
    {
        base.reserve(base.size() + additional.size());
        base.insert(base.end(), std::make_move_iterator(additional.begin()), std::make_move_iterator(additional.end()));
        return base;
    }
}