#pragma once

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/api/mouse_event.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/attributes/impl/attribute_factory.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-switch)
#    include "../../../styles/switch.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    namespace SwitchDetail
    {
        struct IsCheckedReify
        {
            using type = Nui::Attribute;

            static type reify(Nui::StateTransformerBase const&, auto& statefulObject)
            {
                using namespace Nui::Attributes::Literals;
                return "data-is-checked"_attr = statefulObject;
            }
        };
    }

    struct SwitchOptions
    {
        Nui::StateTransformer<SwitchDetail::IsCheckedReify> isChecked = false;
        std::vector<Nui::Attribute> attributes = {};

        std::optional<double> sizeFactor = std::nullopt;
        std::optional<std::string> colorActive = std::nullopt;
        std::optional<std::string> colorInactive = std::nullopt;
        std::optional<std::string> thumbColor = std::nullopt;
        std::function<void(bool, Nui::WebApi::MouseEvent const&)> onChange = {};
        bool dontUpdateValue = false;
    };

    Nui::ElementRenderer switch_(SwitchOptions options);

} // namespace ScriptNuiComponents