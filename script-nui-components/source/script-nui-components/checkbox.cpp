#include <script-nui-components/checkbox.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>

#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/svg_elements.hpp>
#include <nui/frontend/svg_attributes.hpp>

#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/frontend/attributes/type.hpp>

#include <fmt/format.h>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer checkbox(CheckboxOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::span;
        using namespace std::string_literals;
        namespace svge = Nui::Elements::Svg;
        namespace svga = Nui::Attributes::Svg;

        const auto styleVars =
            options.sizeFactor ? fmt::format("--size-factor: {};", *options.sizeFactor) : std::string{};

        const auto [isChecked] = options.isChecked.reify();
        auto [label] = options.label.reify();

        // clang-format off
        return Nui::Elements::button{Nui::mergeAttributes(
            {
                class_ = "script-nui-checkbox",
                type = "button",
                isChecked,
                style = styleVars,
                onClick =
                    [isChecked = options.isChecked,
                        onChange = std::move(options.onChange),
                        dontUpdateValue = options.dontUpdateValue](Nui::WebApi::MouseEvent event) mutable
                {
                    event.stopPropagation();

                    const auto previousValue = isChecked.template value<bool>();

                    if (!dontUpdateValue)
                    {
                        isChecked.assign(!previousValue, Nui::ChangePolicy::Tracked);
                        // The children are pointer events transparent, so currentTarget is not
                        // needed, but target may still be the root when clicked on its padding.
                        event.val()["currentTarget"].call<void>("toggleAttribute", "data-is-checked"s);
                    }

                    if (onChange)
                        onChange(!previousValue, event);
                },
            },
            std::move(options.attributes)
        )}(
            span{class_ = "script-nui-checkbox-box"}(
                svge::svg{
                    svga::viewBox = "0 0 24 24"s,
                }(
                    svge::path{
                        svga::d = "m5 13 4 4 10-10"s,
                    }()
                )
            ),
            span{class_ = "script-nui-checkbox-label"}(std::move(label))
        );
        // clang-format on
    }

} // namespace ScriptNuiComponents
