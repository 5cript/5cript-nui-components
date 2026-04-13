#pragma once

#include <script-nui-components/tree.hpp>

#include <algorithm>
#include <any>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ScriptNuiComponents
{
    /** @brief Folds a flat list of items keyed by `/`-separated relative paths
     *         into a `Tree::Node` forest.
     *
     *  - Intermediate directories are created on demand and given NodeId equal
     *    to their accumulated path WITH a trailing `/` so they never collide
     *    with a leaf at the same path.
     *  - Leaf nodes are created with NodeId equal to the item's relKey verbatim.
     *  - The order of items is preserved as the order in which siblings appear
     *    at each level: walking the input once and inserting in encounter order
     *    keeps the surface predictable for the caller.
     *
     *  @param items           Flat input list.
     *  @param relKeyOf        Returns the relative path for an item.  May
     *                         return string_view; the caller's buffer must
     *                         outlive the call.
     *  @param makeUserData    Builds the std::any payload for a leaf node.
     *  @param makeDirUserData Optional payload builder for synthesised
     *                         directory nodes (called with the directory's
     *                         relKey, with trailing `/` stripped).  When
     *                         empty, directory nodes get an empty std::any.
     */
    template <typename T, typename RelKeyFn, typename MakeLeafDataFn>
    inline std::vector<Tree::Node> foldByRelKey(
        std::vector<T> const& items,
        RelKeyFn const& relKeyOf,
        MakeLeafDataFn const& makeUserData,
        std::function<std::any(std::string_view)> const& makeDirUserData = {}
    )
    {
        std::vector<Tree::Node> roots;
        // dirs[<accumulated path with trailing /, "" for root>] = pointer to
        // the children vector at that directory.  Pointers are stable because
        // we only emplace into the existing vector slot for that directory.
        std::unordered_map<std::string, std::vector<Tree::Node>*> dirs;
        dirs.emplace(std::string{}, &roots);

        // An item whose relKey is itself a directory (e.g. `LocalLinks`) and
        // which ALSO has children in the fold result (e.g. `LocalLinks/foo`)
        // would otherwise produce two tree nodes: one Leaf `LocalLinks` and one
        // Directory `LocalLinks/`.  Merge by moving the leaf's userData onto
        // the matching synthesised directory.
        const auto ensureDirectory =
            [&](std::string const& accumKey, std::string const& parentKey) -> std::vector<Tree::Node>* {
            const auto findIt = dirs.find(accumKey);
            if (findIt != dirs.end())
                return findIt->second;

            auto* const parentVec = dirs[parentKey];
            Tree::Node dirNode{};
            dirNode.id = accumKey; // includes trailing '/'
            dirNode.kind = Tree::NodeKind::Directory;
            if (makeDirUserData)
                dirNode.userData = makeDirUserData(std::string_view{accumKey}.substr(0, accumKey.size() - 1));

            // Look for an existing Leaf sibling whose id is this directory's
            // id minus the trailing slash — that's the same logical entry the
            // caller already emitted as a flat item.  Move its userData onto
            // the new directory so a single row shows both the per-item action
            // and the expandable subtree.
            const std::string_view leafId{accumKey.data(), accumKey.size() - 1};
            auto leafIt = std::find_if(parentVec->begin(), parentVec->end(), [&](Tree::Node const& sibling) {
                return sibling.kind == Tree::NodeKind::Leaf && sibling.id == leafId;
            });
            if (leafIt != parentVec->end())
            {
                dirNode.userData = std::move(leafIt->userData);
                parentVec->erase(leafIt);
            }

            parentVec->push_back(std::move(dirNode));
            auto* const childrenVec = &parentVec->back().children;
            dirs.emplace(accumKey, childrenVec);
            return childrenVec;
        };

        const auto pushLeaf = [&](std::vector<Tree::Node>* siblings, std::string_view relKey, T const& item) {
            // Mirror of ensureDirectory's merge: if a Directory sibling already
            // exists with id `relKey + "/"`, attach the leaf's userData to it
            // and skip inserting a duplicate leaf.
            std::string dirId;
            dirId.reserve(relKey.size() + 1);
            dirId.assign(relKey);
            dirId.push_back('/');
            auto dirIt = std::find_if(siblings->begin(), siblings->end(), [&](Tree::Node const& sibling) {
                return sibling.kind == Tree::NodeKind::Directory && sibling.id == dirId;
            });
            if (dirIt != siblings->end())
            {
                dirIt->userData = makeUserData(item);
                return;
            }

            Tree::Node leaf{};
            leaf.id.assign(relKey);
            leaf.kind = Tree::NodeKind::Leaf;
            leaf.userData = makeUserData(item);
            siblings->push_back(std::move(leaf));
        };

        for (auto const& item : items)
        {
            const std::string_view rel = relKeyOf(item);
            if (rel.empty())
                continue;

            std::string accum;
            std::string parentKey;
            std::vector<Tree::Node>* currentChildren = &roots;

            std::size_t cursor = 0;
            while (cursor < rel.size())
            {
                const auto slash = rel.find('/', cursor);
                if (slash == std::string_view::npos)
                {
                    pushLeaf(currentChildren, rel, item);
                    break;
                }

                const auto segName = rel.substr(cursor, slash - cursor);
                parentKey = accum;
                accum.append(segName.data(), segName.size());
                accum.push_back('/');
                currentChildren = ensureDirectory(accum, parentKey);
                cursor = slash + 1;
            }
        }

        return roots;
    }
}
