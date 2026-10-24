#include <script-nui-components/list_row.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/api/keyboard_event.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/attributes/class.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ScriptNuiComponents
{
    namespace
    {
        bool isActivationKey(std::string const& key)
        {
            return key == "Enter" || key == " ";
        }

        bool isInsideTrailing(Nui::val const& event)
        {
            const auto trailing =
                event["target"].call<Nui::val>("closest", std::string{".script-nui-list-row-trailing"});
            return !trailing.isNull() && event["currentTarget"].call<bool>("contains", trailing);
        }
    } // namespace

    Nui::ElementRenderer listRow(ListRowOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::span;

        auto [label] = options.label.reify();
        auto [sideText] = options.sideText.reify();
        auto [selected, ariaSelected] = options.selected.reify();

        std::vector<Nui::Attribute> rowAttributes;
        rowAttributes.reserve(7u);
        rowAttributes.push_back(class_ = "script-nui-list-row");
        rowAttributes.push_back("role"_attr = "option");
        rowAttributes.push_back("tabindex"_attr = "0");
        rowAttributes.push_back(std::move(selected));
        rowAttributes.push_back(std::move(ariaSelected));
        if (options.onActivate)
        {
            const auto activate = std::make_shared<std::function<void()> const>(std::move(options.onActivate));
            rowAttributes.push_back("click"_event = [activate](Nui::val event) {
                if (isInsideTrailing(event))
                    return;
                (*activate)();
            });
            rowAttributes.push_back("keydown"_event = [activate](Nui::WebApi::KeyboardEvent event) {
                if (!event.target().strictlyEquals(event.currentTarget()) || !isActivationKey(event.key()))
                    return;
                event.preventDefault();
                (*activate)();
            });
        }

        // clang-format off
        return div{Nui::mergeAttributes(std::move(rowAttributes), std::move(options.attributes))}(
            options.leading
                ? span{class_ = "script-nui-list-row-leading"}(std::move(*options.leading))
                : Nui::nil(),
            span{class_ = "script-nui-list-row-label"}(std::move(label)),
            span{class_ = "script-nui-list-row-side"}(std::move(sideText)),
            options.trailing
                ? span{class_ = "script-nui-list-row-trailing"}(std::move(*options.trailing))
                : Nui::nil()
        );
        // clang-format on
    }

} // namespace ScriptNuiComponents
