#pragma once

#include <script-nui-components/style_variant.hpp>
#include <script-nui-components/state_transformers/text_node.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>

#include <ui5-sap-icons/icons/information.hpp>
#include <ui5-sap-icons/icons/warning.hpp>
#include <ui5-sap-icons/icons/error.hpp>
#include <ui5-sap-icons/icons/sys-enter.hpp>

#include <optional>
#include <vector>

namespace ScriptNuiComponents
{
    struct MessageStripOptions
    {
        Nui::StateTransformer<StateTransformers::TextNode> text;
        StyleVariant styleVariant = StyleVariant::Primary;
        std::optional<Nui::ElementRenderer> icon = [this]() -> std::optional<Nui::ElementRenderer>
        {
            if (styleVariant == StyleVariant::Primary)
                return Ui5Icons::information();
            else if (styleVariant == StyleVariant::Warning)
                return Ui5Icons::warning();
            else if (styleVariant == StyleVariant::Danger)
                return Ui5Icons::error();
            else if (styleVariant == StyleVariant::Success)
                return Ui5Icons::sys_enter();
            else
                return std::nullopt;
        }();
        std::vector<Nui::Attribute> attributes = {};
    };

    Nui::ElementRenderer messageStrip(MessageStripOptions options);

} // namespace ScriptNuiComponents