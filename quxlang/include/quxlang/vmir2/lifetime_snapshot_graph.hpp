// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LIFETIME_SNAPSHOT_GRAPH_HEADER_GUARD
#define QUXLANG_VMIR2_LIFETIME_SNAPSHOT_GRAPH_HEADER_GUARD

#include <algorithm>
#include <compare>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace quxlang::vmir2
{
    /** Orders a represented lifetime obligation by SCC rank and source position. */
    struct lifetime_snapshot_position
    {
        std::size_t component = 0;
        std::size_t node = 0;
        std::size_t instruction = 0;

        friend auto operator<=>(lifetime_snapshot_position const&, lifetime_snapshot_position const&) = default;
    };

    /** Covers all represented uses of one shared snapshot or metadata payload. */
    struct lifetime_snapshot_range
    {
        std::optional< lifetime_snapshot_position > first;
        std::optional< lifetime_snapshot_position > last;

        /** Extends the range by one reachable CFG position. */
        void include(lifetime_snapshot_position position)
        {
            first = first.has_value() ? std::min(*first, position) : position;
            last = last.has_value() ? std::max(*last, position) : position;
        }

        /** Extends the range by another represented range. */
        void include(lifetime_snapshot_range const& range)
        {
            if (range.first.has_value()) include(*range.first);
            if (range.last.has_value()) include(*range.last);
        }
    };

    /**
     * Enumerates shared ordered-map snapshots once in deterministic postorder.
     * Pointer identity only identifies existing sharing; traversal order assigns
     * stable indices. The index requires O(D log D) work and O(D) memory for D
     * distinct snapshot nodes, independently of the sum of snapshot sizes.
     */
    template < typename Map >
    class lifetime_snapshot_graph
    {
      public:
        using source_node = typename Map::snapshot_node;
        using snapshot = typename Map::snapshot;

        /** Holds one immutable trie node with children preceding their parent. */
        struct node
        {
            snapshot source;
            std::optional< std::size_t > left;
            std::optional< std::size_t > right;
        };

        /** Registers one snapshot and returns its shared root index. */
        std::optional< std::size_t > append(snapshot root)
        {
            if (!root) return std::nullopt;
            typename std::map< source_node const*, std::size_t >::const_iterator existing = indices_.find(root.get());
            if (existing != indices_.end()) return existing->second;
            std::optional< std::size_t > left = append(root->left);
            std::optional< std::size_t > right = append(root->right);
            std::size_t index = nodes_.size();
            indices_.emplace(root.get(), index);
            nodes_.push_back({.source = std::move(root), .left = left, .right = right});
            return index;
        }

        /** Returns the compact graph without visiting any logical map entries. */
        std::vector< node > const& nodes() const { return nodes_; }

        /** Propagates root obligations through shared nodes with one visit per edge. */
        void propagate(std::vector< lifetime_snapshot_range >& ranges) const
        {
            if (ranges.size() != nodes_.size())
            {
                throw std::invalid_argument("Lifetime snapshot ranges do not match the shared graph");
            }
            for (std::size_t index = nodes_.size(); index != 0; --index)
            {
                node const& current = nodes_[index - 1];
                lifetime_snapshot_range const& range = ranges[index - 1];
                if (current.left.has_value()) ranges[*current.left].include(range);
                if (current.right.has_value()) ranges[*current.right].include(range);
            }
        }

      private:
        std::map< source_node const*, std::size_t > indices_;
        std::vector< node > nodes_;
    };

    /**
     * Rewrites shared lifetime nodes once while preserving ordered key ranges.
     * The callback returns zero or one logical entry for each source leaf and
     * preserves key order. This supports shared filtering and monotonic renaming.
     */
    template < typename Map >
    class lifetime_snapshot_transform
    {
      public:
        using snapshot = typename Map::snapshot;
        using source_node = typename Map::snapshot_node;

        /** Concatenates disjoint ordered snapshots with bounded radix-path copying. */
        static snapshot concatenate(snapshot left, snapshot right)
        {
            if (!left) return right;
            if (!right) return left;
            if (left->maximum_key >= right->minimum_key)
            {
                throw std::invalid_argument("Lifetime snapshot concatenation requires disjoint ordered keys");
            }
            std::uint64_t difference = left->minimum_key ^ right->maximum_key;
            std::uint64_t branch = std::uint64_t{1} << (63 - std::countl_zero(difference));
            std::shared_ptr< source_node > result;
            if (left->branch_bit == branch)
            {
                result = std::make_shared< source_node >(*left);
                result->right = std::const_pointer_cast< source_node >(concatenate(left->right, std::move(right)));
            }
            else if (right->branch_bit == branch)
            {
                result = std::make_shared< source_node >(*right);
                result->left = std::const_pointer_cast< source_node >(concatenate(std::move(left), right->left));
            }
            else
            {
                result = std::make_shared< source_node >();
                result->branch_bit = branch;
                result->left = std::const_pointer_cast< source_node >(std::move(left));
                result->right = std::const_pointer_cast< source_node >(std::move(right));
            }
            result->minimum_key = result->left->minimum_key;
            result->maximum_key = result->right->maximum_key;
            result->count = result->left->count + result->right->count;
            return result;
        }

        /** Publishes a transformed snapshot while retaining all previously visited versions. */
        template < typename Callback >
        snapshot apply(snapshot root, Callback const& callback)
        {
            if (!root) return {};
            typename std::map< source_node const*, std::pair< snapshot, snapshot > >::const_iterator previous = transformed_.find(root.get());
            if (previous != transformed_.end()) return previous->second.second;
            snapshot result;
            if (root->value)
            {
                std::optional< typename Map::value_type > value = callback(*root->value);
                if (value.has_value())
                {
                    if (*value == *root->value) result = root;
                    else
                    {
                        Map singleton;
                        singleton.emplace(value->first, std::move(value->second));
                        result = singleton.snapshot_root();
                    }
                }
            }
            else
            {
                snapshot left = apply(root->left, callback);
                snapshot right = apply(root->right, callback);
                if (left == root->left && right == root->right) result = root;
                else result = concatenate(std::move(left), std::move(right));
            }
            transformed_.emplace(root.get(), std::pair{root, result});
            return result;
        }

      private:
        std::map< source_node const*, std::pair< snapshot, snapshot > > transformed_;
    };
}

#endif // QUXLANG_VMIR2_LIFETIME_SNAPSHOT_GRAPH_HEADER_GUARD
