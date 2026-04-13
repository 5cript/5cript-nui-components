#include <script-nui-components/button.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>

#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/text.hpp>

#include <nui/frontend/attributes/disabled.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/style.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer button(ButtonOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;

        std::vector<Nui::Attribute> baseAttributes;
        baseAttributes.reserve(3);

        if (options.shine && options.shine->trigger)
        {
            auto* trigger = options.shine->trigger;
            baseAttributes.push_back(class_ = observe(*trigger).generate([trigger]() -> std::string {
                const int value = trigger->value();
                // Initial value (0) must NOT carry a snc-shine-* class — the
                // animation would otherwise play on mount.  The class is only
                // added once the caller explicitly bumps the trigger (>=1).
                if (value <= 0)
                    return std::string{"script-nui-button snc-shinable"};
                return (value & 1)
                    ? std::string{"script-nui-button snc-shinable snc-shine-b"}
                    : std::string{"script-nui-button snc-shinable snc-shine-a"};
            }));
            if (!options.shine->color.empty())
            {
                baseAttributes.push_back(
                    style = std::string{"--snc-shine-color: "} + options.shine->color + ";"
                );
            }
        }
        else
        {
            baseAttributes.push_back(class_ = "script-nui-button");
        }
        baseAttributes.push_back(std::get<0>(options.styleVariant.reify()));

        auto btn = Nui::Elements::button{mergeAttributes(
            std::move(baseAttributes),
            std::move(options.attributes)
        )};

        auto [text] = options.text.reify();

        if (options.icon)
            return std::move(btn)(*options.icon, std::move(text));
        return std::move(btn)(std::move(text));
    }

} // namespace ScriptNuiComponents
