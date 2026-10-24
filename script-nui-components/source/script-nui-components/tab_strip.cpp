#include <script-nui-components/tab_strip.hpp>

#include <nui/frontend/utility/merge_attributes.hpp>
#include <nui/frontend/api/keyboard_event.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/attributes/class.hpp>
#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/event_system/range.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ScriptNuiComponents
{
    namespace
    {
        /**
         * @brief What the tabs of one strip share.
         */
        struct TabStripState
        {
            std::vector<std::string> keys;
            std::shared_ptr<Nui::Observed<std::string>> selected;
            std::function<void(std::string const& key)> onSelect;
        };

        bool isSelected(TabStripState const& state, std::size_t index)
        {
            return state.selected->value() == state.keys[index];
        }

        bool isFocusTarget(TabStripState const& state, std::size_t index)
        {
            if (isSelected(state, index))
                return true;
            return index == 0u && std::ranges::find(state.keys, state.selected->value()) == state.keys.end();
        }

        void assignSelection(TabStripState const& state, std::size_t index)
        {
            if (!isSelected(state, index))
                *state.selected = state.keys[index];
        }

        void notifySelection(TabStripState const& state, std::size_t index)
        {
            if (state.onSelect)
                state.onSelect(state.keys[index]);
        }

        std::optional<std::size_t> keyTargetOf(Nui::WebApi::KeyboardEvent const& event, std::size_t index, std::size_t count)
        {
            if (event.altKey() || event.ctrlKey() || event.metaKey())
                return std::nullopt;
            const auto key = event.key();
            if (key == "ArrowLeft")
                return (index + count - 1u) % count;
            if (key == "ArrowRight")
                return (index + 1u) % count;
            if (key == "Home")
                return 0u;
            if (key == "End")
                return count - 1u;
            return std::nullopt;
        }

        void focusSibling(Nui::val const& tab, std::size_t index)
        {
            const auto sibling = tab["parentElement"]["children"].call<Nui::val>("item", static_cast<int>(index));
            if (!sibling.isNull() && !sibling.isUndefined())
                sibling.call<void>("focus");
        }

        Nui::ElementRenderer renderTab(std::shared_ptr<TabStripState const> const& state, std::size_t index, TabStripTab tab)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::span;

            const auto weakState = std::weak_ptr<TabStripState const>{state};
            const auto whenAlive = [weakState, index](auto predicate) {
                return [weakState, index, predicate]() {
                    const auto locked = weakState.lock();
                    return locked && predicate(*locked, index);
                };
            };
            const auto selectedNow = whenAlive(isSelected);
            const auto focusTargetNow = whenAlive(isFocusTarget);

            auto [title] = tab.title.reify();

            std::vector<Nui::Attribute> tabAttributes;
            tabAttributes.reserve(9u);
            tabAttributes.push_back(class_ = "script-nui-tab-strip-tab");
            tabAttributes.push_back("type"_attr = "button");
            tabAttributes.push_back("role"_attr = "tab");
            tabAttributes.push_back("data-tab-key"_attr = tab.key);
            tabAttributes.push_back("aria-selected"_attr = Nui::observe(state->selected).generate([selectedNow]() {
                return std::string{selectedNow() ? "true" : "false"};
            }));
            tabAttributes.push_back("data-selected"_attr = Nui::observe(state->selected).generate(selectedNow));
            tabAttributes.push_back("tabindex"_attr = Nui::observe(state->selected).generate([focusTargetNow]() {
                return std::string{focusTargetNow() ? "0" : "-1"};
            }));
            tabAttributes.push_back("click"_event = [state, index]() {
                assignSelection(*state, index);
                notifySelection(*state, index);
            });
            tabAttributes.push_back("keydown"_event = [state, index](Nui::WebApi::KeyboardEvent event) {
                const auto target = keyTargetOf(event, index, state->keys.size());
                if (!target)
                    return;
                event.preventDefault();
                assignSelection(*state, *target);
                focusSibling(event.currentTarget(), *target);
                notifySelection(*state, *target);
            });

            // clang-format off
            return button{Nui::mergeAttributes(std::move(tabAttributes), std::move(tab.attributes))}(
                span{class_ = "script-nui-tab-strip-title"}(std::move(title)),
                tab.badge
                    ? countBadge(std::move(tab.badge->count), tab.badge->styleVariant)
                    : Nui::nil()
            );
            // clang-format on
        }
    } // namespace

    Nui::ElementRenderer tabStrip(TabStripOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        std::vector<std::string> keys;
        keys.reserve(options.tabs.size());
        for (auto const& tab : options.tabs)
            keys.push_back(tab.key);

        auto selected = options.selected ? std::move(options.selected) : std::make_shared<Nui::Observed<std::string>>();
        const auto state = std::make_shared<TabStripState const>(TabStripState{
            .keys = std::move(keys),
            .selected = std::move(selected),
            .onSelect = std::move(options.onSelect),
        });

        std::vector<Nui::ElementRenderer> tabs;
        tabs.reserve(options.tabs.size());
        for (std::size_t index = 0u; index < options.tabs.size(); ++index)
            tabs.push_back(renderTab(state, index, std::move(options.tabs[index])));

        std::vector<Nui::Attribute> stripAttributes;
        stripAttributes.reserve(3u);
        stripAttributes.push_back(class_ = "script-nui-tab-strip");
        stripAttributes.push_back("role"_attr = "tablist");
        if (!options.label.empty())
            stripAttributes.push_back("aria-label"_attr = std::move(options.label));

        // clang-format off
        return div{Nui::mergeAttributes(std::move(stripAttributes), std::move(options.attributes))}(
            Nui::range(std::move(tabs)),
            [](long long, Nui::ElementRenderer const& tab) -> Nui::ElementRenderer {
                return tab;
            }
        );
        // clang-format on
    }

} // namespace ScriptNuiComponents
