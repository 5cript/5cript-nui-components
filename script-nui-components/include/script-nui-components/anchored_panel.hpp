#pragma once

#include <script-nui-components/style_variant.hpp>

#include <nui/frontend/element_renderer.hpp>

#include <memory>
#include <string>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-anchored-panel)
#    include "../../../styles/anchored_panel.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /** @brief A small non-modal panel that positions itself absolutely inside
     *         its nearest positioned ancestor.  The host element MUST carry
     *         `position: relative` (or any non-static positioning) for the
     *         panel to anchor correctly; otherwise it will escape up the
     *         containing-block chain.
     *
     *  Not derived from Dialog to avoid regressing the viewport-modal
     *  semantics of `<dialog>`.  Has no built-in buttons — the caller composes
     *  them inside the body renderer.
     */
    class AnchoredPanel
    {
      public:
        AnchoredPanel(std::string id, Nui::ElementRenderer body);

        ~AnchoredPanel();
        AnchoredPanel(AnchoredPanel const&) = delete;
        AnchoredPanel(AnchoredPanel&&);
        AnchoredPanel& operator=(AnchoredPanel const&) = delete;
        AnchoredPanel& operator=(AnchoredPanel&&);

        struct OpenOptions
        {
            StyleVariant styleVariant = StyleVariant::Regular;
            std::string headerText = "";
        };

        /** @brief Show the panel with the given options. */
        void open(OpenOptions const& options);

        /** @brief Hide the panel. */
        void close();

        /** @brief Whether the panel is currently open. */
        bool isOpen() const;

        Nui::ElementRenderer operator()();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
} // namespace ScriptNuiComponents
