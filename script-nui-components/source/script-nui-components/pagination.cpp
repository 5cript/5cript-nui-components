#include <script-nui-components/pagination.hpp>

#include <ui5-sap-icons/icons/slim-arrow-left.hpp>
#include <ui5-sap-icons/icons/slim-arrow-right.hpp>
#include <ui5-sap-icons/icons/overflow.hpp>

#include <nui/frontend/elements/div.hpp>
#include <nui/frontend/elements/span.hpp>
#include <nui/frontend/elements/button.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/event_system/range.hpp>

#include <nui/frontend/attributes/class.hpp>
#include <nui/frontend/attributes/on_click.hpp>
#include <nui/frontend/attributes/disabled.hpp>

#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/utility/assert.hpp>

#include <algorithm>
#include <set>
#include <vector>

namespace ScriptNuiComponents
{
    namespace
    {
        /**
         * @brief Compute the visible page indices for the numbered portion of the bar.
         *        Always includes first and last, current +/- 1. Gaps between indices
         *        are rendered as overflow-icon spans by the caller.
         */
        std::vector<int> visiblePages(int pageCount, int currentPage)
        {
            NUI_ASSERT(pageCount >= 1, "pagination: pageCount must be >= 1");
            NUI_ASSERT(currentPage >= 0, "pagination: currentPage must be >= 0");
            NUI_ASSERT(currentPage < pageCount, "pagination: currentPage must be < pageCount");

            std::set<int> indices;
            indices.insert(0);
            if (pageCount != 1) // the set would de-dupe, but preempt it explicitly
                indices.insert(pageCount - 1);

            for (int delta = -1; delta <= 1; ++delta)
            {
                const int candidate = currentPage + delta;
                if (candidate >= 0 && candidate < pageCount)
                    indices.insert(candidate);
            }
            return {indices.begin(), indices.end()};
        }

        Nui::ElementRenderer renderPageButton(
            int pageIndex,
            int pageCount,
            int currentPage,
            std::string const& liveSuffix,
            std::function<void(int)> const& onPageChange
        )
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::span;
            using Nui::Elements::button;

            const bool isSelected = pageIndex == currentPage;
            const bool isLive = pageIndex == pageCount - 1;

            std::string classes = "script-nui-pagination-page";
            if (isSelected)
                classes += " selected";
            if (isLive)
                classes += " live";

            const std::string label = std::to_string(pageIndex + 1) + (isLive ? liveSuffix : std::string{});

            return button{
                class_ = std::move(classes),
                onClick = [pageIndex, onPageChange](Nui::val)
                {
                    if (onPageChange)
                        onPageChange(pageIndex);
                },
            }(span{}(label));
        }

        Nui::ElementRenderer renderOverflowGap()
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::span;
            return span{class_ = "script-nui-pagination-gap"}(Ui5Icons::overflow());
        }

        Nui::ElementRenderer
        renderNavButton(bool isPrev, int currentPage, int pageCount, std::function<void(int)> const& onPageChange)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::button;

            const bool disabledState = isPrev ? (currentPage == 0) : (currentPage == pageCount - 1);
            std::string classes = "script-nui-pagination-nav";
            if (disabledState)
                classes += " disabled";

            return button{
                class_ = std::move(classes),
                disabled = disabledState ? std::optional<bool>{true} : std::nullopt,
                onClick = [isPrev, currentPage, pageCount, onPageChange](Nui::val)
                {
                    if (!onPageChange)
                        return;
                    if (isPrev && currentPage > 0)
                        onPageChange(currentPage - 1);
                    else if (!isPrev && currentPage < pageCount - 1)
                        onPageChange(currentPage + 1);
                },
            }(isPrev ? Ui5Icons::slim_arrow_left() : Ui5Icons::slim_arrow_right());
        }

        std::vector<Nui::ElementRenderer> buildBarChildren(
            int pageCount,
            int currentPage,
            std::string const& liveSuffix,
            std::function<void(int)> const& onPageChange
        )
        {
            std::vector<Nui::ElementRenderer> children;
            children.reserve(static_cast<std::size_t>(pageCount) + 4);

            children.push_back(renderNavButton(true, currentPage, pageCount, onPageChange));

            const auto pageIndices = visiblePages(pageCount, currentPage);
            int previous = -1;
            for (int pageIndex : pageIndices)
            {
                if (previous >= 0 && pageIndex > previous + 1)
                    children.push_back(renderOverflowGap());
                children.push_back(renderPageButton(pageIndex, pageCount, currentPage, liveSuffix, onPageChange));
                previous = pageIndex;
            }

            children.push_back(renderNavButton(false, currentPage, pageCount, onPageChange));
            return children;
        }
    } // namespace

    Nui::ElementRenderer pagination(PaginationOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        if (!options.pageCount || !options.currentPage)
            return Nui::nil();

        auto* pageCountObs = options.pageCount;
        auto* currentPageObs = options.currentPage;
        auto onPageChange = std::move(options.onPageChange);
        const std::string liveSuffix =
            options.liveLabel.has_value() && !options.liveLabel->empty() ? "-" + *options.liveLabel : std::string{};

        return div{
            class_ = "script-nui-pagination"
        }(Nui::observe(*pageCountObs, *currentPageObs),
            [pageCountObs, currentPageObs, onPageChange, liveSuffix]() -> Nui::ElementRenderer
            {
                const int pageCount = pageCountObs->value();
                if (pageCount <= 1)
                    return Nui::nil();

                const int currentPage = std::clamp(currentPageObs->value(), 0, pageCount - 1);
                auto children = buildBarChildren(pageCount, currentPage, liveSuffix, onPageChange);

                // std::move into range() to select the owning rvalue overload
                // — a plain range(lvalue) would capture the vector by reference
                // and dangle as soon as this lambda returns.
                return div{
                    class_ = "script-nui-pagination-bar"
                }(Nui::range(std::move(children)),
                    [](long long, auto const& elem) -> Nui::ElementRenderer
                    {
                        return elem;
                    });
            });
    }
} // namespace ScriptNuiComponents
