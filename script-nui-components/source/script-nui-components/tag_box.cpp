#include <script-nui-components/tag_box.hpp>
#include <script-nui-components/pill.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/keyboard_event.hpp>
#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/event_system/range.hpp>
#include <nui/event_system/observed_value.hpp>

#include <algorithm>
#include <cctype>
#include <ranges>

namespace ScriptNuiComponents
{
    namespace
    {
        std::string trim(std::string value)
        {
            const auto space = [](unsigned char c) { return std::isspace(c) != 0; };
            auto front = std::ranges::find_if_not(value, space);
            auto back = std::ranges::find_if_not(value | std::views::reverse, space).base();
            std::string result(front, back);
            // A trailing comma is the separator key; strip it and any whitespace it hid.
            if (!result.empty() && result.back() == ',')
            {
                result.pop_back();
                result.erase(std::ranges::find_if_not(result | std::views::reverse, space).base(), result.end());
            }
            return result;
        }
    }

    struct TagBox::Implementation
    {
        Nui::Observed<std::vector<std::string>> tags;
        std::string placeholder;
        std::function<void(std::vector<std::string> const&)> onChange;

        explicit Implementation(Options options)
            : tags{std::move(options.initialTags)}
            , placeholder{std::move(options.placeholder)}
            , onChange{std::move(options.onChange)}
        {}

        void notify() const
        {
            if (onChange)
                onChange(tags.value());
        }

        void addTag(std::string raw)
        {
            auto tag = trim(std::move(raw));
            if (tag.empty())
                return;
            auto const& current = tags.value();
            if (std::ranges::find(current, tag) != current.end())
                return;
            tags.push_back(std::move(tag));
            notify();
        }

        void removeTag(std::string const& tag)
        {
            auto proxy = tags.modify();
            proxy->erase(std::remove(proxy->begin(), proxy->end(), tag), proxy->end());
            notify();
        }
    };

    TagBox::TagBox(Options options)
        : impl_{std::make_unique<Implementation>(std::move(options))}
    {}
    TagBox::~TagBox() = default;
    TagBox::TagBox(TagBox&&) = default;
    TagBox& TagBox::operator=(TagBox&&) = default;

    std::vector<std::string> TagBox::tags() const
    {
        return impl_->tags.value();
    }

    void TagBox::tags(std::vector<std::string> value)
    {
        impl_->tags = std::move(value);
    }

    void TagBox::clear()
    {
        impl_->tags.clear();
    }

    Nui::ElementRenderer TagBox::operator()(std::vector<Nui::Attribute> extraAttributes)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;
        using Nui::Elements::input;

        return div{mergeAttributes(
            std::vector<Nui::Attribute>{class_ = "script-nui-tag-box"},
            std::move(extraAttributes)
        )}(
            Nui::range(impl_->tags)
                .after(input{
                    class_ = "script-nui-tag-box-input",
                    "type"_attr = "text",
                    "placeholder"_attr = impl_->placeholder,
                    // Uncontrolled input: read the text from the DOM target on commit and clear
                    // it directly. Backspace is intentionally left to normal text editing so it
                    // never deletes a tag by accident.
                    onKeyDown =
                        [this](Nui::WebApi::KeyboardEvent event) {
                            const auto key = event.key();
                            if (key != "Enter" && key != ",")
                                return;
                            event.preventDefault();
                            auto target = event.val()["target"];
                            impl_->addTag(target["value"].as<std::string>());
                            target.set("value", std::string{});
                        },
                }()),
            [this](long long, auto const& tag) -> Nui::ElementRenderer {
                return pill({
                    .text = tag,
                    .onRemove = [this, tag]() { impl_->removeTag(tag); },
                });
            }
        );
    }

} // namespace ScriptNuiComponents
