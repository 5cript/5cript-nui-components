#include <script-nui-components/message_strip.hpp>

#include <nui/frontend/elements/section.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/nil.hpp>

#include <nui/frontend/attributes/class.hpp>

namespace ScriptNuiComponents
{
    Nui::ElementRenderer messageStrip(MessageStripOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::span;

        return section{Nui::mergeAttributes(
            std::move(options.attributes),
            {"data-style-variant"_attr = toString(options.styleVariant), class_ = "script-nui-message-strip"}
        )}(options.icon ? *options.icon : Nui::nil(), span{}(std::get<0>(options.text.reify())));
    }

} // namespace ScriptNuiComponents