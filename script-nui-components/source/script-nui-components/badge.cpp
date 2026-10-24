#include <script-nui-components/badge.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/attributes/class.hpp>

#include <string>
#include <utility>
#include <vector>

namespace ScriptNuiComponents
{
    namespace
    {
        std::string popNameOf(int pop)
        {
            if (pop <= 0)
                return {};
            return pop % 2 == 1 ? "b" : "a";
        }
    } // namespace

    Nui::ElementRenderer badge(BadgeOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::span;

        auto [text] = options.text.reify();
        auto [variant] = options.styleVariant.reify();
        auto [hidden] = options.hidden.reify();

        std::vector<Nui::Attribute> attributes;
        attributes.reserve(5u);
        attributes.push_back(class_ = "script-nui-badge");
        attributes.push_back(std::move(variant));
        attributes.push_back(std::move(hidden));
        if (options.dot)
            attributes.push_back("data-dot"_attr = true);
        if (options.pop != nullptr)
        {
            attributes.push_back("data-pop"_attr = Nui::observe(*options.pop).generate([](int pop) {
                return popNameOf(pop);
            }));
        }

        return span{Nui::mergeAttributes(std::move(attributes), std::move(options.attributes))}(
            options.dot ? Nui::nil() : std::move(text)
        );
    }

    Nui::ElementRenderer countBadge(
        BadgeCount count,
        Nui::StateTransformer<StateTransformers::StyleVariantTransformer> styleVariant,
        std::vector<Nui::Attribute> attributes
    )
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::span;

        auto [text, hidden] = count.reify();
        auto [variant] = styleVariant.reify();

        std::vector<Nui::Attribute> rootAttributes;
        rootAttributes.reserve(3u);
        rootAttributes.push_back(class_ = "script-nui-badge");
        rootAttributes.push_back(std::move(variant));
        rootAttributes.push_back(std::move(hidden));

        return span{Nui::mergeAttributes(std::move(rootAttributes), std::move(attributes))}(std::move(text));
    }

} // namespace ScriptNuiComponents
