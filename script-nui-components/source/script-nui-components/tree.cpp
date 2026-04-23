#include <script-nui-components/tree.hpp>

#include <nui/event_system/event_context.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/elements/nil.hpp>
#include <nui/frontend/elements/svg/path.hpp>
#include <nui/frontend/elements/svg/svg.hpp>
#include <nui/frontend/attributes/svg/d.hpp>
#include <nui/frontend/attributes/svg/view_box.hpp>
#include <nui/frontend/api/event.hpp>
#include <nui/frontend/api/mouse_event.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <functional>
#include <list>
#include <optional>
#include <utility>

namespace ScriptNuiComponents
{
    namespace
    {
        constexpr char const* kRootParentId = "";

        /** @brief A small inline SVG triangle pointing right.  CSS rotates it
         *         90deg when the row is `--expanded`.
         */
        Nui::ElementRenderer chevronGlyph()
        {
            using namespace Nui::Elements::Svg;
            using namespace Nui::Attributes::Svg;
            using namespace Nui::Attributes;
            return svg{
                viewBox = "0 0 16 16",
                "aria-hidden"_attr = "true",
            }(path{d = "M5 3 L11 8 L5 13 Z"}());
        }

        /** @brief Two stacked chevrons pointing up — collapse-all icon. */
        Nui::ElementRenderer collapseAllGlyph()
        {
            using namespace Nui::Elements::Svg;
            using namespace Nui::Attributes::Svg;
            using namespace Nui::Attributes;
            return svg{
                viewBox = "0 0 16 16",
                "aria-hidden"_attr = "true",
            }(
                path{d = "M3 9 L8 4 L13 9", "fill"_attr = "none",
                     "stroke"_attr = "currentColor", "stroke-width"_attr = "2",
                     "stroke-linecap"_attr = "round", "stroke-linejoin"_attr = "round"}(),
                path{d = "M3 13 L8 8 L13 13", "fill"_attr = "none",
                     "stroke"_attr = "currentColor", "stroke-width"_attr = "2",
                     "stroke-linecap"_attr = "round", "stroke-linejoin"_attr = "round"}()
            );
        }

        /** @brief Filled square with a checkmark — select-all icon. */
        Nui::ElementRenderer selectAllGlyph()
        {
            using namespace Nui::Elements::Svg;
            using namespace Nui::Attributes::Svg;
            using namespace Nui::Attributes;
            return svg{
                viewBox = "0 0 16 16",
                "aria-hidden"_attr = "true",
            }(
                path{d = "M2 3 L14 3 L14 13 L2 13 Z", "fill"_attr = "none",
                     "stroke"_attr = "currentColor", "stroke-width"_attr = "1.5"}(),
                path{d = "M5 8 L7 10 L11 6", "fill"_attr = "none",
                     "stroke"_attr = "currentColor", "stroke-width"_attr = "2",
                     "stroke-linecap"_attr = "round", "stroke-linejoin"_attr = "round"}()
            );
        }

        /** @brief Empty square with a diagonal slash — deselect-all icon. */
        Nui::ElementRenderer deselectAllGlyph()
        {
            using namespace Nui::Elements::Svg;
            using namespace Nui::Attributes::Svg;
            using namespace Nui::Attributes;
            return svg{
                viewBox = "0 0 16 16",
                "aria-hidden"_attr = "true",
            }(
                path{d = "M2 3 L14 3 L14 13 L2 13 Z", "fill"_attr = "none",
                     "stroke"_attr = "currentColor", "stroke-width"_attr = "1.5"}(),
                path{d = "M4 12 L12 4", "fill"_attr = "none",
                     "stroke"_attr = "currentColor", "stroke-width"_attr = "2",
                     "stroke-linecap"_attr = "round"}()
            );
        }
    }

    struct Tree::Implementation
    {
        Options options;

        struct NodeRecord
        {
            Node data{};
            NodeId parent{};
            // Children NodeIds in display order.  Populated for Directory nodes
            // (eagerly during setRoots, lazily after a successful loader call).
            // The original `data.children` is moved out / left empty after
            // registration to avoid duplication.
            std::vector<NodeId> childOrder{};
            bool everExpanded{false};
            std::size_t pageCursor{0};
            enum class Load
            {
                NotStarted,
                Loading,
                Loaded,
                Failed,
            } loadState{Load::NotStarted};
            std::string loadError{};
            bool inLru{false};
            std::list<NodeId>::iterator lruIt{};
        };

        struct NodeObs
        {
            // Observed used by the children-slot renderer to gate the FIRST
            // mount of the children container.  Once true, it stays true until
            // a prune flips it back to false.
            Nui::Observed<bool> everExpanded{false};
            // Drives `display: none` on the existing children container.  Cheap
            // toggle, no DOM rebuild.
            Nui::Observed<bool> expanded{false};
            // The visible window of `childOrder` indices, capped to
            // `pageCursor + pageSize`.  Strictly additive grow on loadMore().
            Nui::Observed<std::vector<std::size_t>> pageIndices{};
            // Bumped whenever loadState/loadError change so the skeleton/error
            // renderer re-runs.
            Nui::Observed<int> loadTick{0};
            // Bumped whenever the load-more affordance changes visibility.
            Nui::Observed<int> loadMoreTick{0};
        };

        std::unordered_map<NodeId, NodeRecord> nodes{};
        std::unordered_map<NodeId, std::shared_ptr<NodeObs>> obs{};
        std::vector<NodeId> rootOrder{};
        // Holds page indices for the synthetic root level (id = kRootParentId).
        std::shared_ptr<NodeObs> rootObs{std::make_shared<NodeObs>()};
        std::list<NodeId> collapsedLru{};

        // ──────────────────────── helpers ────────────────────────────────────

        std::shared_ptr<NodeObs> getOrCreateObs(NodeId const& id)
        {
            auto findIt = obs.find(id);
            if (findIt != obs.end())
                return findIt->second;
            auto fresh = std::make_shared<NodeObs>();
            obs.emplace(id, fresh);
            return fresh;
        }

        std::vector<NodeId>& childOrderOf(NodeId const& parentId)
        {
            if (parentId == kRootParentId)
                return rootOrder;
            return nodes.at(parentId).childOrder;
        }

        std::shared_ptr<NodeObs> obsOf(NodeId const& id)
        {
            if (id == kRootParentId)
                return rootObs;
            return getOrCreateObs(id);
        }

        std::size_t totalChildren(NodeId const& parentId) const
        {
            if (parentId == kRootParentId)
                return rootOrder.size();
            const auto findIt = nodes.find(parentId);
            return findIt == nodes.end() ? 0u : findIt->second.childOrder.size();
        }

        // ──────────────────────── registration ───────────────────────────────

        /** @brief Recursively registers a Node (and descendants) in `nodes`.
         *         Moves the original `Node::children` out into separate records,
         *         so the caller's tree of values flattens into the per-id map.
         */
        void registerNode(Node node, NodeId const& parentId)
        {
            const auto id = node.id;

            NodeRecord rec{};
            rec.parent = parentId;
            rec.data.id = node.id;
            rec.data.kind = node.kind;
            rec.data.hasChildren = node.hasChildren;
            rec.data.userData = std::move(node.userData);
            rec.data.icon = std::move(node.icon);
            rec.data.initiallyExpanded = node.initiallyExpanded;
            rec.data.selectable = node.selectable;

            // Children inline → register and remember the order.
            std::vector<Node> incomingChildren = std::move(node.children);
            rec.childOrder.reserve(incomingChildren.size());
            for (auto& child : incomingChildren)
                rec.childOrder.push_back(child.id);

            const bool hasInlineChildren = !incomingChildren.empty();
            if (rec.data.kind == NodeKind::Directory && hasInlineChildren)
            {
                rec.data.hasChildren = true;
                // Children data is present but not yet rendered: the node is
                // still "never expanded" until the user clicks its chevron.
                rec.loadState = NodeRecord::Load::Loaded;
            }

            const bool wantInitial = node.initiallyExpanded && rec.data.kind == NodeKind::Directory;

            // Insert (or replace) the record.  If a previous record exists,
            // preserve its runtime state so expand/collapse survives recompares.
            // `everExpanded` is sticky-once-true (marks "children DOM mounted");
            // the actual visible/hidden state is the observed `expanded` flag,
            // which is the one the user has been toggling.  Read it before the
            // record is replaced so we can restore it below.
            const auto prevIt = nodes.find(id);
            const bool hadPrev = prevIt != nodes.end();
            bool previousExpanded = false;
            bool lazyReseed = false;
            if (hadPrev)
            {
                rec.pageCursor = prevIt->second.pageCursor;
                lazyReseed =
                    rec.data.kind == NodeKind::Directory && !hasInlineChildren &&
                    prevIt->second.loadState == NodeRecord::Load::Loaded;
                // Lazy re-seed (caller passes children={} for a directory whose
                // previous record already had children loaded) wipes the child
                // list — so also reset everExpanded / loadState / loadError,
                // otherwise @ref expand short-circuits the childrenLoader and
                // the row renders empty.
                if (lazyReseed)
                {
                    rec.everExpanded = false;
                    rec.loadState = NodeRecord::Load::NotStarted;
                    rec.loadError.clear();
                }
                else
                {
                    rec.everExpanded = prevIt->second.everExpanded;
                    if (hasInlineChildren)
                        rec.loadState = NodeRecord::Load::Loaded;
                    else
                        rec.loadState = prevIt->second.loadState;
                    rec.loadError = prevIt->second.loadError;
                }
                if (prevIt->second.inLru)
                {
                    collapsedLru.erase(prevIt->second.lruIt);
                }
                const auto prevObsIt = obs.find(id);
                if (prevObsIt != obs.end())
                    previousExpanded = prevObsIt->second->expanded.value();
            }
            else if (wantInitial)
            {
                rec.everExpanded = true;
            }
            nodes.insert_or_assign(id, std::move(rec));

            // Recurse.  After this call, the inline-children Node objects are
            // discarded — only their identifiers and descendants remain in `nodes`.
            for (auto& child : incomingChildren)
                registerNode(std::move(child), id);

            // Sync observed state and rebuild the page indices for this node.
            auto& observedNode = *getOrCreateObs(id);
            auto& finalRec = nodes.at(id);
            // On a lazy re-seed we must also force the visible `expanded` bit
            // back to false — the children DOM has been implicitly un-mounted
            // (everExpanded=false) and leaving the chevron in its previous
            // expanded state would render an empty, stuck-open row.
            const bool startExpanded =
                lazyReseed ? false : (hadPrev ? previousExpanded : wantInitial);

            if (observedNode.everExpanded.value() != finalRec.everExpanded)
                observedNode.everExpanded = finalRec.everExpanded;
            if (observedNode.expanded.value() != startExpanded)
                observedNode.expanded = startExpanded;
            rebuildPageIndices(id);
        }

        void rebuildPageIndices(NodeId const& parentId)
        {
            auto observedParent = obsOf(parentId);
            const auto total = totalChildren(parentId);
            std::size_t cursor = options.pageSize;
            if (parentId != kRootParentId)
            {
                const auto findIt = nodes.find(parentId);
                if (findIt != nodes.end())
                    cursor = findIt->second.pageCursor + options.pageSize;
            }
            const std::size_t visible = std::min(total, std::max<std::size_t>(cursor, options.pageSize));
            observedParent->pageIndices.value().resize(visible);
            for (std::size_t i = 0; i < visible; ++i)
                observedParent->pageIndices.value()[i] = i;
            observedParent->pageIndices.modify();
            observedParent->loadMoreTick = observedParent->loadMoreTick.value() + 1;
        }

        // ──────────────────────── expand / collapse ──────────────────────────

        void expand(NodeId const& id)
        {
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end() || findIt->second.data.kind != NodeKind::Directory)
                return;
            auto& rec = findIt->second;
            auto observedNode = getOrCreateObs(id);

            if (rec.inLru)
            {
                collapsedLru.erase(rec.lruIt);
                rec.inLru = false;
            }

            const bool needsLoad = !rec.everExpanded && rec.childOrder.empty() && rec.data.hasChildren &&
                options.childrenLoader && rec.loadState != NodeRecord::Load::Loading;

            if (needsLoad)
            {
                rec.loadState = NodeRecord::Load::Loading;
                rec.loadError.clear();
                observedNode->loadTick = observedNode->loadTick.value() + 1;
                observedNode->everExpanded = true;
                observedNode->expanded = true;
                rec.everExpanded = true; // children container will be mounted now (showing skeleton)
                Nui::globalEventContext.executeActiveEventsImmediately();

                auto loader = *options.childrenLoader;
                loader(
                    id,
                    [this, id](std::vector<Node> resolved) {
                        onChildrenResolved(id, std::move(resolved));
                    },
                    [this, id](std::string err) {
                        onChildrenFailed(id, std::move(err));
                    }
                );
                return;
            }

            rec.everExpanded = true;
            observedNode->everExpanded = true;
            observedNode->expanded = true;
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        void collapse(NodeId const& id)
        {
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end() || findIt->second.data.kind != NodeKind::Directory)
                return;
            auto& rec = findIt->second;
            auto observedNode = getOrCreateObs(id);

            observedNode->expanded = false;

            if (!rec.everExpanded)
                return; // never showed children — nothing to LRU-track

            if (!rec.inLru)
            {
                rec.lruIt = collapsedLru.insert(collapsedLru.end(), id);
                rec.inLru = true;
            }

            maybeEvictOldestCollapsed();

            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        void maybeEvictOldestCollapsed()
        {
            if (options.maxCachedCollapsed == 0)
                return;
            while (collapsedLru.size() > options.maxCachedCollapsed)
            {
                // Try to find the oldest evictable subtree (i.e., one whose
                // descendants don't intersect the selection).  If none qualify,
                // bail out — never silently drop selection state.
                auto victimIt = std::find_if(collapsedLru.begin(), collapsedLru.end(), [this](NodeId const& candidate) {
                    return !subtreeIntersectsSelection(candidate);
                });
                if (victimIt == collapsedLru.end())
                    return;
                const auto victimId = *victimIt;
                collapsedLru.erase(victimIt);
                pruneSubtree(victimId);
            }
        }

        bool subtreeIntersectsSelection(NodeId const& id) const
        {
            if (!options.selected)
                return false;
            auto const& selectedSet = options.selected->value();
            if (selectedSet.empty())
                return false;

            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return false;
            for (auto const& childId : findIt->second.childOrder)
            {
                if (selectedSet.contains(childId))
                    return true;
                if (subtreeIntersectsSelection(childId))
                    return true;
            }
            return false;
        }

        /** @brief Drops cached children of @p id (recursively), rebuilds its
         *         observed state to "collapsed and never expanded" so a future
         *         expand re-instantiates the children DOM (and re-loads via
         *         loader if applicable).
         */
        void pruneSubtree(NodeId const& id)
        {
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return;
            auto& rec = findIt->second;

            for (auto const& childId : rec.childOrder)
                eraseSubtree(childId);
            rec.childOrder.clear();
            rec.everExpanded = false;
            rec.pageCursor = 0;
            rec.inLru = false;
            // For lazy-loaded directories, clear loaded flag so the loader runs again.
            if (options.childrenLoader && rec.data.hasChildren)
                rec.loadState = NodeRecord::Load::NotStarted;

            auto observedNode = getOrCreateObs(id);
            observedNode->everExpanded = false;
            observedNode->expanded = false;
            observedNode->pageIndices.value().clear();
            observedNode->pageIndices.modify();
            observedNode->loadTick = observedNode->loadTick.value() + 1;
        }

        void eraseSubtree(NodeId const& id)
        {
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return;
            auto childOrderCopy = findIt->second.childOrder;
            // Remove from LRU if present.
            if (findIt->second.inLru)
                collapsedLru.erase(findIt->second.lruIt);
            nodes.erase(findIt);
            obs.erase(id);
            for (auto const& childId : childOrderCopy)
                eraseSubtree(childId);
        }

        // ──────────────────────── lazy children resolve ──────────────────────

        void onChildrenResolved(NodeId const& id, std::vector<Node> children)
        {
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return;
            auto& rec = findIt->second;
            rec.childOrder.clear();
            rec.childOrder.reserve(children.size());
            for (auto& child : children)
                rec.childOrder.push_back(child.id);
            for (auto& child : children)
                registerNode(std::move(child), id);
            rec.loadState = NodeRecord::Load::Loaded;
            rec.everExpanded = true;
            auto observedNode = getOrCreateObs(id);
            observedNode->loadTick = observedNode->loadTick.value() + 1;
            rebuildPageIndices(id);
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        void onChildrenFailed(NodeId const& id, std::string error)
        {
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return;
            auto& rec = findIt->second;
            rec.loadState = NodeRecord::Load::Failed;
            rec.loadError = std::move(error);
            auto observedNode = getOrCreateObs(id);
            observedNode->loadTick = observedNode->loadTick.value() + 1;
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        // ──────────────────────── pagination ─────────────────────────────────

        void loadMore(NodeId const& parentId)
        {
            if (parentId == kRootParentId)
            {
                const auto total = rootOrder.size();
                const auto current = rootObs->pageIndices.value().size();
                const auto next = std::min(total, current + options.pageSize);
                for (auto i = current; i < next; ++i)
                    rootObs->pageIndices.push_back(i);
                rootObs->loadMoreTick = rootObs->loadMoreTick.value() + 1;
            }
            else
            {
                const auto findIt = nodes.find(parentId);
                if (findIt == nodes.end())
                    return;
                auto& rec = findIt->second;
                rec.pageCursor += options.pageSize;
                auto observedNode = getOrCreateObs(parentId);
                const auto total = rec.childOrder.size();
                const auto current = observedNode->pageIndices.value().size();
                const auto next = std::min(total, current + options.pageSize);
                for (auto i = current; i < next; ++i)
                    observedNode->pageIndices.push_back(i);
                observedNode->loadMoreTick = observedNode->loadMoreTick.value() + 1;
            }
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        // ──────────────────────── selection / cascade ────────────────────────

        void forEachLeafDescendant(NodeId const& id, std::function<void(NodeId const&)> const& visit)
        {
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return;
            auto const& rec = findIt->second;
            if (rec.data.kind == NodeKind::Leaf)
            {
                visit(rec.data.id);
                return;
            }
            for (auto const& childId : rec.childOrder)
                forEachLeafDescendant(childId, visit);
        }

        void toggleRowSelection(NodeId const& id, bool nowSelected)
        {
            if (!options.selected)
                return;
            auto& selectedSet = options.selected->value();

            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return;

            // Caller-provided override — takes full responsibility for the
            // selection mutation.  Used for sparse-set semantics where a single
            // entry implies a whole subtree.
            if (options.toggleSelection)
            {
                options.toggleSelection(id, nowSelected, selectedSet);
                options.selected->modify();
                Nui::globalEventContext.executeActiveEventsImmediately();
                return;
            }

            if (findIt->second.data.kind == NodeKind::Leaf)
            {
                if (nowSelected)
                    selectedSet.insert(id);
                else
                    selectedSet.erase(id);
                if (options.onSelectionChanged)
                    options.onSelectionChanged(id, nowSelected);
            }
            else
            {
                forEachLeafDescendant(id, [&](NodeId const& leafId) {
                    if (nowSelected)
                        selectedSet.insert(leafId);
                    else
                        selectedSet.erase(leafId);
                    if (options.onSelectionChanged)
                        options.onSelectionChanged(leafId, nowSelected);
                });
            }
            options.selected->modify();
            Nui::globalEventContext.executeActiveEventsImmediately();
        }

        using TriState = SelectionState;

        TriState computeSelectionState(NodeId const& id) const
        {
            if (!options.selected)
                return TriState::Unchecked;
            // Caller override — full responsibility for the tri-state value.
            if (options.selectionStateResolver)
                return options.selectionStateResolver(id);
            auto const& selectedSet = options.selected->value();
            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return TriState::Unchecked;
            if (findIt->second.data.kind == NodeKind::Leaf)
                return selectedSet.contains(id) ? TriState::Checked : TriState::Unchecked;

            std::size_t total = 0;
            std::size_t selectedCount = 0;
            // Walk recursively; counts only loaded descendants.
            std::function<void(NodeId const&)> walk = [&](NodeId const& nodeId) {
                const auto innerFindIt = nodes.find(nodeId);
                if (innerFindIt == nodes.end())
                    return;
                if (innerFindIt->second.data.kind == NodeKind::Leaf)
                {
                    ++total;
                    if (selectedSet.contains(nodeId))
                        ++selectedCount;
                    return;
                }
                for (auto const& childId : innerFindIt->second.childOrder)
                    walk(childId);
            };
            walk(id);

            if (total == 0)
                return TriState::Unchecked;
            if (selectedCount == 0)
                return TriState::Unchecked;
            if (selectedCount == total)
                return TriState::Checked;
            return TriState::Indeterminate;
        }

        // ──────────────────────── render ─────────────────────────────────────

        Nui::ElementRenderer renderRow(NodeId const& id, std::size_t depth)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;
            using Nui::Elements::span;

            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return Nui::nil();
            auto const& rec = findIt->second;
            const bool isDir = rec.data.kind == NodeKind::Directory;
            const bool hasChildren = isDir && (rec.data.hasChildren || !rec.childOrder.empty());

            auto observedNode = getOrCreateObs(id);

            // Chevron
            auto chevronCell = [&]() -> Nui::ElementRenderer {
                if (!hasChildren)
                {
                    return span{
                        class_ = "script-nui-tree__chevron script-nui-tree__chevron--hidden",
                        "aria-hidden"_attr = "true",
                    }();
                }
                return span{
                    class_ = Nui::observe(observedNode->expanded).generate([sharedObs = observedNode]() {
                        return sharedObs->expanded.value()
                            ? std::string{"script-nui-tree__chevron script-nui-tree__chevron--expanded"}
                            : std::string{"script-nui-tree__chevron script-nui-tree__chevron--collapsed"};
                    }),
                    "aria-hidden"_attr = "true",
                    onClick = [this, id](Nui::WebApi::MouseEvent event) {
                        event.stopPropagation();
                        const auto findInnerIt = nodes.find(id);
                        if (findInnerIt == nodes.end())
                            return;
                        auto innerObs = getOrCreateObs(id);
                        if (innerObs->expanded.value())
                            collapse(id);
                        else
                            expand(id);
                    },
                }(chevronGlyph());
            }();

            // Optional checkbox
            auto checkboxCell = [&]() -> Nui::ElementRenderer {
                if (!options.showCheckboxes)
                    return Nui::nil();
                if (!options.selected)
                    return Nui::nil();
                auto selectedPtr = options.selected;
                // std::optional makes the attribute conditionally present:
                // nullopt => attribute omitted, value => attribute set.
                const std::optional<bool> disabledIfLocked =
                    rec.data.selectable ? std::nullopt : std::optional<bool>{true};
                return span{class_ = "script-nui-tree__checkbox"}(input{
                    type = "checkbox",
                    "checked"_prop = Nui::observe(*selectedPtr).generate([this, id]() {
                        return computeSelectionState(id) == TriState::Checked;
                    }),
                    "indeterminate"_prop = Nui::observe(*selectedPtr).generate([this, id]() {
                        return computeSelectionState(id) == TriState::Indeterminate;
                    }),
                    disabled = disabledIfLocked,
                    onClick = [](Nui::WebApi::MouseEvent event) {
                        event.stopPropagation();
                    },
                    "change"_event = [this, id](Nui::WebApi::Event event) {
                        const bool nowChecked = event.val()["target"]["checked"].as<bool>();
                        toggleRowSelection(id, nowChecked);
                    },
                }());
            }();

            // Optional icon
            auto iconCell = [&]() -> Nui::ElementRenderer {
                if (!options.showIcons || !rec.data.icon)
                    return Nui::nil();
                return span{class_ = "script-nui-tree__icon"}(*rec.data.icon);
            }();

            // Caller-supplied content
            const RowContext ctx{
                .id = id,
                .depth = depth,
                .expanded = observedNode->expanded.value(),
                .hasChildren = hasChildren,
                .userData = rec.data.userData,
            };
            auto contentCell = options.rowContent
                ? options.rowContent(ctx)
                : Nui::nil();

            const std::string depthStyle = fmt::format("--depth: {};", depth);
            // aria-expanded reflects the directory's current expansion state at
            // render time; omitted on leaves via std::optional<std::string>.  It
            // does not auto-update when expanded flips — acceptable for v1; the
            // chevron's visual state is the authoritative UI cue.  Revisit if
            // full screen-reader support is needed.
            const std::optional<std::string> ariaExpandedAttr = isDir
                ? std::optional<std::string>{observedNode->expanded.value() ? "true" : "false"}
                : std::nullopt;

            std::vector<Nui::Attribute> rowAttrs;
            rowAttrs.reserve(6);
            rowAttrs.push_back(class_ = "script-nui-tree__row");
            rowAttrs.push_back(role = "treeitem");
            rowAttrs.push_back("aria-level"_attr = std::to_string(depth + 1));
            rowAttrs.push_back("aria-expanded"_attr = ariaExpandedAttr);
            rowAttrs.push_back(style = depthStyle);
            rowAttrs.push_back(tabIndex = "-1");
            if (options.rowAttributes)
            {
                auto extra = options.rowAttributes(ctx);
                for (auto& attr : extra)
                    rowAttrs.push_back(std::move(attr));
            }

            return div{std::move(rowAttrs)}(
                std::move(chevronCell),
                std::move(checkboxCell),
                std::move(iconCell),
                div{class_ = "script-nui-tree__content"}(std::move(contentCell))
            );
        }

        Nui::ElementRenderer renderChildrenSlot(NodeId const& parentId, std::size_t childDepth)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            auto observedParent = obsOf(parentId);

            return div{class_ = "script-nui-tree__children-slot"}(
                Nui::observe(observedParent->everExpanded),
                [this, parentId, childDepth, sharedObs = observedParent](bool const& ever) -> Nui::ElementRenderer {
                    if (!ever)
                        return Nui::nil();
                    return renderChildrenContainer(parentId, childDepth);
                }
            );
        }

        Nui::ElementRenderer renderChildrenContainer(NodeId const& parentId, std::size_t childDepth)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            auto observedParent = obsOf(parentId);
            auto containerStyle = Nui::observe(observedParent->expanded).generate([sharedObs = observedParent]() {
                return sharedObs->expanded.value() ? std::string{} : std::string{"display: none;"};
            });

            return div{
                class_ = "script-nui-tree__children",
                role = "group",
                style = std::move(containerStyle),
            }(
                div{class_ = "script-nui-tree__list-slot"}(
                    Nui::observe(observedParent->loadTick),
                    [this, parentId, childDepth](int const&) -> Nui::ElementRenderer {
                        return renderChildrenBody(parentId, childDepth);
                    }
                )
            );
        }

        /** @brief Loading / error state, or the page-indexed row list.  Swapped
         *         in by the outer loadTick observer so we don't have to rebuild
         *         the expand/collapse DOM when the load state changes.
         */
        Nui::ElementRenderer renderChildrenBody(NodeId const& parentId, std::size_t childDepth)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            const bool isRootParent = (parentId == kRootParentId);
            if (!isRootParent)
            {
                const auto findIt = nodes.find(parentId);
                if (findIt != nodes.end())
                {
                    if (findIt->second.loadState == NodeRecord::Load::Loading)
                    {
                        const std::string depthStyle = fmt::format("--depth: {};", childDepth);
                        return div{class_ = "script-nui-tree__skeleton", style = depthStyle}("Loading…");
                    }
                    if (findIt->second.loadState == NodeRecord::Load::Failed)
                    {
                        const std::string depthStyle = fmt::format("--depth: {};", childDepth);
                        return div{class_ = "script-nui-tree__error", style = depthStyle}(
                            fmt::format("Failed: {}", findIt->second.loadError)
                        );
                    }
                }
            }

            auto observedParent = obsOf(parentId);
            return div{class_ = "script-nui-tree__rows-wrap"}(
                div{class_ = "script-nui-tree__rows"}(
                    Nui::range(observedParent->pageIndices),
                    [this, parentId, childDepth](long long /*idx*/, std::size_t const& slot) -> Nui::ElementRenderer {
                        auto const& siblings = childOrderOf(parentId);
                        if (slot >= siblings.size())
                            return Nui::nil();
                        return renderNode(siblings[slot], childDepth);
                    }
                ),
                renderLoadMoreSlot(parentId, childDepth)
            );
        }

        Nui::ElementRenderer renderLoadMoreSlot(NodeId const& parentId, std::size_t childDepth)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            auto observedParent = obsOf(parentId);
            return div{class_ = "script-nui-tree__load-more-slot"}(
                Nui::observe(observedParent->loadMoreTick),
                [this, parentId, childDepth, sharedObs = observedParent](int) -> Nui::ElementRenderer {
                    const auto visible = sharedObs->pageIndices.value().size();
                    const auto total = totalChildren(parentId);
                    if (visible >= total)
                        return Nui::nil();
                    const auto remaining = total - visible;
                    const std::string depthStyle = fmt::format("--depth: {};", childDepth);
                    const std::string label =
                        fmt::format("Load up to {} more ({} remaining)", options.pageSize, remaining);
                    return div{
                        class_ = "script-nui-tree__load-more",
                        style = depthStyle,
                        onClick = [this, parentId](Nui::WebApi::MouseEvent event) {
                            event.stopPropagation();
                            loadMore(parentId);
                        },
                    }(label);
                }
            );
        }

        Nui::ElementRenderer renderNode(NodeId const& id, std::size_t depth)
        {
            using namespace Nui::Elements;
            using namespace Nui::Attributes;
            using Nui::Elements::div;

            const auto findIt = nodes.find(id);
            if (findIt == nodes.end())
                return Nui::nil();
            const bool isDir = findIt->second.data.kind == NodeKind::Directory;

            if (!isDir)
            {
                return div{class_ = "script-nui-tree__node"}(renderRow(id, depth));
            }
            return div{class_ = "script-nui-tree__node"}(
                renderRow(id, depth),
                renderChildrenSlot(id, depth + 1)
            );
        }
    };

    // ──────────────────────────── public API ─────────────────────────────────

    Tree::Tree()
        : impl_{std::make_unique<Implementation>()}
    {}

    Tree::Tree(Options options)
        : impl_{std::make_unique<Implementation>()}
    {
        impl_->options = std::move(options);
    }

    Tree::~Tree() = default;
    Tree::Tree(Tree&&) = default;
    Tree& Tree::operator=(Tree&&) = default;

    void Tree::setRoots(std::vector<Node> roots)
    {
        // Identify which previously-known nodes are still in the new root set.
        // Anything not referenced (transitively) gets erased.
        std::unordered_set<NodeId> kept;
        std::function<void(Node const&)> collectKept = [&](Node const& node) {
            kept.insert(node.id);
            for (auto const& child : node.children)
                collectKept(child);
        };
        for (auto const& root : roots)
            collectKept(root);

        // Erase nodes that are no longer present.
        for (auto it = impl_->nodes.begin(); it != impl_->nodes.end();)
        {
            if (!kept.contains(it->first))
            {
                if (it->second.inLru)
                    impl_->collapsedLru.erase(it->second.lruIt);
                impl_->obs.erase(it->first);
                it = impl_->nodes.erase(it);
            }
            else
            {
                ++it;
            }
        }

        impl_->rootOrder.clear();
        impl_->rootOrder.reserve(roots.size());
        for (auto& root : roots)
            impl_->rootOrder.push_back(root.id);
        for (auto& root : roots)
            impl_->registerNode(std::move(root), kRootParentId);

        // Roots' page indices.
        const auto rootCount = impl_->rootOrder.size();
        const auto visible = std::min(rootCount, impl_->options.pageSize);
        impl_->rootObs->pageIndices.value().resize(visible);
        for (std::size_t i = 0; i < visible; ++i)
            impl_->rootObs->pageIndices.value()[i] = i;
        impl_->rootObs->pageIndices.modify();
        impl_->rootObs->everExpanded = true;
        impl_->rootObs->expanded = true;
        impl_->rootObs->loadMoreTick = impl_->rootObs->loadMoreTick.value() + 1;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    void Tree::expand(NodeId const& id)
    {
        impl_->expand(id);
    }
    void Tree::collapse(NodeId const& id)
    {
        impl_->collapse(id);
    }
    void Tree::setExpanded(NodeId const& id, bool expanded)
    {
        if (expanded)
            impl_->expand(id);
        else
            impl_->collapse(id);
    }

    void Tree::collapseAll()
    {
        // Snapshot first — collapse() mutates `obs` indirectly via LRU bookkeeping.
        std::vector<NodeId> toCollapse;
        toCollapse.reserve(impl_->obs.size());
        for (auto const& [id, observedNode] : impl_->obs)
        {
            if (id.empty())
                continue;  // skip the synthetic root entry
            if (observedNode->expanded.value())
                toCollapse.push_back(id);
        }
        for (auto const& id : toCollapse)
            impl_->collapse(id);
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    void Tree::selectAll()
    {
        if (!impl_->options.selected)
            return;
        auto& selectedSet = impl_->options.selected->value();
        if (impl_->options.selectAllAction)
        {
            impl_->options.selectAllAction(selectedSet);
            impl_->options.selected->modify();
            Nui::globalEventContext.executeActiveEventsImmediately();
            return;
        }
        for (auto const& [id, rec] : impl_->nodes)
        {
            if (rec.data.kind != NodeKind::Leaf)
                continue;
            if (!rec.data.selectable)
                continue;
            const bool inserted = selectedSet.insert(id).second;
            if (inserted && impl_->options.onSelectionChanged)
                impl_->options.onSelectionChanged(id, true);
        }
        impl_->options.selected->modify();
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    void Tree::deselectAll()
    {
        if (!impl_->options.selected)
            return;
        auto& selectedSet = impl_->options.selected->value();
        if (impl_->options.deselectAllAction)
        {
            impl_->options.deselectAllAction(selectedSet);
            impl_->options.selected->modify();
            Nui::globalEventContext.executeActiveEventsImmediately();
            return;
        }
        if (impl_->options.onSelectionChanged)
        {
            // Snapshot the IDs before clearing so callbacks see a stable set.
            std::vector<NodeId> previouslySelected{selectedSet.begin(), selectedSet.end()};
            selectedSet.clear();
            for (auto const& id : previouslySelected)
                impl_->options.onSelectionChanged(id, false);
        }
        else
        {
            selectedSet.clear();
        }
        impl_->options.selected->modify();
        Nui::globalEventContext.executeActiveEventsImmediately();
    }

    std::vector<Tree::NodeId> Tree::childrenOf(NodeId const& parent) const
    {
        // Root-level order lives outside the nodes map (synthetic root).
        if (parent.empty())
            return impl_->rootOrder;
        const auto findIt = impl_->nodes.find(parent);
        if (findIt == impl_->nodes.end())
            return {};
        return findIt->second.childOrder;
    }

    Nui::ElementRenderer Tree::operator()(std::vector<Nui::Attribute> additionalAttributes)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // Merge caller-supplied attributes from Options + the per-call overrides.
        std::vector<Nui::Attribute> attrs;
        attrs.reserve(impl_->options.attributes.size() + additionalAttributes.size() + 2);
        attrs.push_back(
            class_ = impl_->options.mirror ? std::string{"script-nui-tree script-nui-tree--mirrored"}
                                           : std::string{"script-nui-tree"});
        attrs.push_back(role = "tree");
        for (auto& attr : impl_->options.attributes)
            attrs.push_back(attr);
        for (auto& attr : additionalAttributes)
            attrs.push_back(std::move(attr));

        auto* impl = impl_.get();

        const bool wantSelectionButtons = impl->options.selected
            && (impl->options.showSelectAllButton || impl->options.showDeselectAllButton);
        const bool hasToolbar = impl->options.showCollapseAllButton || wantSelectionButtons;

        auto toolbarButton = [](Nui::ElementRenderer glyph,
                                std::string const& titleText,
                                std::function<void()> onClickHandler) -> Nui::ElementRenderer {
            return Nui::Elements::button{
                class_ = "script-nui-tree__toolbar-button",
                "type"_attr = "button",
                "title"_attr = titleText,
                "aria-label"_attr = titleText,
                onClick = [handler = std::move(onClickHandler)](Nui::val event) {
                    event.call<void>("stopPropagation");
                    handler();
                },
            }(std::move(glyph));
        };

        Nui::ElementRenderer toolbar = Nui::nil();
        if (hasToolbar)
        {
            toolbar = div{class_ = "script-nui-tree__toolbar"}(
                impl->options.showCollapseAllButton
                    ? toolbarButton(collapseAllGlyph(), "Collapse all", [this]() { collapseAll(); })
                    : Nui::nil(),
                (impl->options.showSelectAllButton && impl->options.selected)
                    ? toolbarButton(selectAllGlyph(), "Select all", [this]() { selectAll(); })
                    : Nui::nil(),
                (impl->options.showDeselectAllButton && impl->options.selected)
                    ? toolbarButton(deselectAllGlyph(), "Deselect all", [this]() { deselectAll(); })
                    : Nui::nil()
            );
        }

        return div{std::move(attrs)}(
            std::move(toolbar),
            div{class_ = "script-nui-tree__rows"}(
                Nui::range(impl->rootObs->pageIndices),
                [impl](long long /*idx*/, std::size_t const& slot) -> Nui::ElementRenderer {
                    if (slot >= impl->rootOrder.size())
                        return Nui::nil();
                    return impl->renderNode(impl->rootOrder[slot], 0);
                }
            ),
            impl->renderLoadMoreSlot(kRootParentId, 0)
        );
    }
}
