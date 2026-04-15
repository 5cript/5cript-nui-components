#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/observed_value.hpp>

#include <functional>
#include <optional>
#include <string>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-pagination)
#    include "../../../styles/pagination.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /**
     * @brief Slim numbered pagination bar with prev/next arrows and overflow-gap icons.
     * @param pageCount   Total number of pages. Bar renders empty when value is <= 1.
     * @param currentPage Currently-selected page index (0-based). Last page is styled as "live".
     * @param onPageChange Called with the new page index when the user clicks a page button.
     * @param liveLabel   Optional suffix appended to the last page ("9-Live" by default).
     */
    struct PaginationOptions
    {
        Nui::Observed<int>* pageCount = nullptr;
        Nui::Observed<int>* currentPage = nullptr;
        std::function<void(int newPage)> onPageChange = {};
        std::optional<std::string> liveLabel = std::string{"Live"};
    };

    Nui::ElementRenderer pagination(PaginationOptions options);
} // namespace ScriptNuiComponents
