#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/event_system/observed_value.hpp>

#include <any>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#if defined(NUI_INLINE) && !defined(SCRIPT_NUI_COMPONENTS_NO_INLINE)
// clang-format off
// @inline(css, script-nui-components-tree)
#    include "../../../styles/tree.css"
// @endinline
// clang-format on
#endif

namespace ScriptNuiComponents
{
    /** @brief A lazy, paginated, optionally-checkboxed tree component.
     *
     *  Designed for two use cases:
     *    - eager: caller passes the full tree via setRoots() (sync dialog).
     *    - lazy : caller provides only roots with `hasChildren=true,children={}`
     *             plus an `Options::childrenLoader` that fetches a node's
     *             children the first time it is expanded (file explorer).
     *
     *  Collapse keeps the subtree mounted; first expand of a node is lazy.
     *  Optional LRU pruning (`Options::maxCachedCollapsed`) caps memory of
     *  collapsed-but-cached subtrees.  Children past `Options::pageSize` are
     *  paginated behind a "Load more" sentinel that the user clicks.
     */
    class Tree
    {
      public:
        /// Caller-supplied stable identity of a node.  Opaque to the tree.
        using NodeId = std::string;

        enum class NodeKind
        {
            Leaf,
            Directory,
        };

        struct Node
        {
            NodeId id;
            NodeKind kind{NodeKind::Leaf};
            std::vector<Node> children{};
            /// Set to true on a Directory with empty children to force a lazy
            /// load via Options::childrenLoader on first expand.
            bool hasChildren{false};
            std::any userData{};
            std::optional<Nui::ElementRenderer> icon{};
            bool initiallyExpanded{false};
            /// When false, the row's checkbox is disabled (still rendered if
            /// Options::showCheckboxes is true, just non-interactive).
            bool selectable{true};
        };

        struct RowContext
        {
            NodeId const& id;
            std::size_t depth;
            bool expanded;
            bool hasChildren;
            std::any const& userData;
        };

        using RowContentRenderer = std::function<Nui::ElementRenderer(RowContext const&)>;
        /// Extra attributes to apply to the outer row `<div>` for a node.  Use
        /// this to decorate the whole row (progress overlay, custom class,
        /// data-* flags) without having to wrap the content cell.
        using RowAttributeProvider = std::function<std::vector<Nui::Attribute>(RowContext const&)>;

        using ChildrenLoader = std::function<void(
            NodeId const& parent,
            std::function<void(std::vector<Node>)> resolve,
            std::function<void(std::string)> reject
        )>;

        using SelectionChanged = std::function<void(NodeId const&, bool selected)>;

        struct Options
        {
            /// REQUIRED. Renders the row's content cell.
            RowContentRenderer rowContent{};
            /// OPTIONAL. Extra attributes merged into the outer row `<div>`.
            RowAttributeProvider rowAttributes{};
            /// OPTIONAL. When set, directory nodes with empty `children` AND
            /// `hasChildren==true` lazily fetch via this callback on first expand.
            std::optional<ChildrenLoader> childrenLoader{};
            bool showCheckboxes{false};
            bool showIcons{true};
            /// Mirror the tree horizontally: chevron, indent, and guide lines
            /// render on the right instead of the left.  Handy when the caller's
            /// row content is right-aligned (e.g. the "Download" section of a
            /// diff view where the meaningful text sits on the remote side).
            bool mirror{false};
            std::size_t pageSize{200};
            /// Cap on cached-but-collapsed subtrees.  When exceeded, the LRU
            /// (oldest collapsedAt) cached subtree is evicted inline at the
            /// moment of collapse.  0 = never prune.  Skips subtrees whose
            /// descendants intersect `selected`.
            std::size_t maxCachedCollapsed{0};
            /// Selection surface — the same set is read AND written by the
            /// tree.  Callers may also mutate it externally.
            std::shared_ptr<Nui::Observed<std::unordered_set<NodeId>>> selected{};
            SelectionChanged onSelectionChanged{};
            std::vector<Nui::Attribute> attributes{};
            /// Render a built-in collapse-all icon button in the tree toolbar.
            bool showCollapseAllButton{false};
            /// Render a built-in select-all icon button in the tree toolbar.
            /// Has no effect when @ref selected is null.
            bool showSelectAllButton{false};
            /// Render a built-in deselect-all icon button in the tree toolbar.
            /// Has no effect when @ref selected is null.
            bool showDeselectAllButton{false};
        };

        Tree();
        explicit Tree(Options options);
        ~Tree();
        Tree(Tree const&) = delete;
        Tree& operator=(Tree const&) = delete;
        Tree(Tree&&);
        Tree& operator=(Tree&&);

        /** @brief Replace the tree's root list.  Performs a keyed merge against
         *         existing nodes (matched by NodeId): runtime state (everExpanded,
         *         expanded, pageCursor, loadState) is preserved for surviving
         *         nodes, dropped for removed ones, fresh for newly added ones.
         */
        void setRoots(std::vector<Node> roots);

        void expand(NodeId const& id);
        void collapse(NodeId const& id);
        void setExpanded(NodeId const& id, bool expanded);

        /** @brief Collapses every currently-expanded directory.  Cached children
         *         remain mounted (no prune); a future expand reveals them
         *         instantly without re-running any lazy loader.
         */
        void collapseAll();

        /** @brief Adds every leaf NodeId to @ref Options::selected (no-op if
         *         selected is null).  Fires onSelectionChanged once per leaf.
         */
        void selectAll();

        /** @brief Clears @ref Options::selected (no-op if selected is null).
         *         Fires onSelectionChanged once per previously-selected leaf.
         */
        void deselectAll();

        Nui::ElementRenderer operator()(std::vector<Nui::Attribute> additionalAttributes = {});

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}
