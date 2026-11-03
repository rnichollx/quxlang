// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LIFETIME_STATE_DIFFERENCE_HEADER_GUARD
#define QUXLANG_VMIR2_LIFETIME_STATE_DIFFERENCE_HEADER_GUARD

#include <quxlang/vmir2/lifetime_state_map.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace quxlang::vmir2
{
    /**
     * Visits source entries whose target versions differ, in descending key order.
     * Equal shared subtrees require one comparison. Each remaining source or
     * target branch is visited at most once, apart from the bounded radix depth
     * needed to align differently shaped trees. Target-only entries are omitted.
     * The return value counts visited node pairs for deterministic work accounting.
     */
    template < typename Key, typename Value, typename Visitor >
    auto visit_lifetime_state_difference(lifetime_state_map< Key, Value > const& source,
        lifetime_state_map< Key, Value > const& target, Visitor&& visitor) -> std::size_t
    {
        using node = typename lifetime_state_map< Key, Value >::snapshot_node;
        typename lifetime_state_map< Key, Value >::snapshot source_root = source.snapshot_root();
        typename lifetime_state_map< Key, Value >::snapshot target_root = target.snapshot_root();
        std::vector< std::pair< node const*, node const* > > pending{{source_root.get(), target_root.get()}};
        std::size_t work = 0;
        while (!pending.empty())
        {
            std::pair< node const*, node const* > current = pending.back();
            pending.pop_back();
            ++work;
            node const* left = current.first;
            node const* right = current.second;
            if (left == nullptr || left == right) continue;
            if (right != nullptr && (left->maximum_key < right->minimum_key || right->maximum_key < left->minimum_key))
            {
                right = nullptr;
            }
            if (left->value)
            {
                while (right != nullptr && !right->value)
                {
                    ++work;
                    right = (left->minimum_key & right->branch_bit) == 0 ? right->left.get() : right->right.get();
                }
                if (right == nullptr || left->minimum_key != right->minimum_key)
                {
                    visitor(*left->value, static_cast< Value const* >(nullptr));
                }
                else if (left->value != right->value)
                {
                    visitor(*left->value, &right->value->second);
                }
                continue;
            }
            if (right == nullptr || left->branch_bit > right->branch_bit)
            {
                // The target occupies at most one side of the source's split.
                node const* lower = right != nullptr && (right->minimum_key & left->branch_bit) == 0 ? right : nullptr;
                node const* upper = right != nullptr && (right->minimum_key & left->branch_bit) != 0 ? right : nullptr;
                pending.emplace_back(left->left.get(), lower);
                pending.emplace_back(left->right.get(), upper);
            }
            else if (left->branch_bit < right->branch_bit)
            {
                pending.emplace_back(left, (left->minimum_key & right->branch_bit) == 0 ? right->left.get() : right->right.get());
            }
            else
            {
                pending.emplace_back(left->left.get(), right->left.get());
                pending.emplace_back(left->right.get(), right->right.get());
            }
        }
        return work;
    }
}

#endif // QUXLANG_VMIR2_LIFETIME_STATE_DIFFERENCE_HEADER_GUARD
