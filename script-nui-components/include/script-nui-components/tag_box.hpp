#pragma once

#include <nui/frontend/element_renderer.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-tag-box)
#    include "../../../styles/tag_box.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /**
     * @brief An editable tag input: type + Enter/comma to add a tag, click the x to remove one,
     *        Backspace on an empty field deletes the last tag.
     */
    class TagBox
    {
      public:
        struct Options
        {
            std::vector<std::string> initialTags = {};
            std::string placeholder = {};
            /// Invoked after every user edit (add/remove) with the full current tag list.
            std::function<void(std::vector<std::string> const&)> onChange = {};
        };

        explicit TagBox(Options options);
        ~TagBox();

        TagBox(TagBox const&) = delete;
        TagBox& operator=(TagBox const&) = delete;
        TagBox(TagBox&&);
        TagBox& operator=(TagBox&&);

        /// Current tags (snapshot copy).
        std::vector<std::string> tags() const;
        /// Replace all tags (does not fire onChange; leaves syncing to the caller).
        void tags(std::vector<std::string> value);
        /// Remove all tags (does not fire onChange).
        void clear();

        Nui::ElementRenderer operator()(std::vector<Nui::Attribute> extraAttributes = {});

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };

} // namespace ScriptNuiComponents
