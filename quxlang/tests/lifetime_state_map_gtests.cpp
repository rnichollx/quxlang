// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/lifetime_state_map.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace
{
    using map_type = quxlang::vmir2::lifetime_state_map< std::uint64_t, std::uint64_t >;

    /** Counts unique nodes across snapshots without flattening their shared subtrees. */
    std::size_t snapshot_nodes(std::vector< map_type > const& maps)
    {
        std::set< map_type::snapshot_node const* > visited;
        std::vector< map_type::snapshot > pending;
        for (map_type const& map : maps) pending.push_back(map.snapshot_root());
        while (!pending.empty())
        {
            map_type::snapshot current = std::move(pending.back());
            pending.pop_back();
            if (!current || !visited.insert(current.get()).second) continue;
            if (current->left) pending.push_back(current->left);
            if (current->right) pending.push_back(current->right);
        }
        return visited.size();
    }
}

TEST(lifetime_state_map, snapshots_preserve_independent_versions)
{
    map_type original;
    original[4] = 40;
    original[1] = 10;
    original[9] = 90;
    map_type updated = original;
    updated[4] = 41;
    updated.erase(1);
    updated[12] = 120;
    EXPECT_EQ(original.at(4), 40);
    EXPECT_EQ(original.at(1), 10);
    EXPECT_FALSE(original.contains(12));
    EXPECT_EQ(updated.at(4), 41);
    EXPECT_FALSE(updated.contains(1));
    EXPECT_EQ(updated.at(12), 120);
}

TEST(lifetime_state_map, sparse_and_extreme_keys_preserve_order)
{
    map_type values;
    std::map< std::uint64_t, std::uint64_t > expected;
    for (std::uint64_t key : std::array< std::uint64_t, 7 >{0, 7, std::uint64_t{1} << 63, 63, std::numeric_limits< std::uint64_t >::max(), std::uint64_t{1} << 32, 1})
    {
        values[key] = key ^ 37;
        expected[key] = key ^ 37;
    }
    EXPECT_EQ(values.serialize_interface(), expected);
    std::vector< std::uint64_t > reverse;
    for (map_type::const_reverse_iterator entry = values.crbegin(); entry != values.crend(); ++entry) reverse.push_back(entry->first);
    std::vector< std::uint64_t > expected_reverse;
    for (std::pair< std::uint64_t const, std::uint64_t > const& entry : expected) expected_reverse.insert(expected_reverse.begin(), entry.first);
    EXPECT_EQ(reverse, expected_reverse);
    for (std::pair< std::uint64_t const, std::uint64_t > const& entry : expected)
    {
        EXPECT_EQ(values.erase(entry.first), 1);
        EXPECT_EQ(values.erase(entry.first), 0);
    }
    EXPECT_TRUE(values.empty());
}

TEST(lifetime_state_map, iterators_retain_their_snapshot_during_mutation)
{
    map_type values{{1, 11}, {2, 22}, {3, 33}};
    map_type::const_iterator old = values.begin();
    values[1] = 99;
    values.erase(2);
    EXPECT_EQ(old->second, 11);
    ++old;
    EXPECT_EQ(old->first, 2);
    EXPECT_EQ(old->second, 22);
    ++old;
    EXPECT_EQ(old->first, 3);
    ++old;
    EXPECT_EQ(old, values.end());
}

TEST(lifetime_state_map, many_large_snapshots_share_unchanged_storage)
{
    map_type values;
    constexpr std::size_t count = 4096;
    for (std::size_t index = 0; index < count; ++index) values[index] = index;
    std::vector< map_type > snapshots;
    for (std::size_t index = 0; index < count; ++index)
    {
        snapshots.push_back(values);
        values[index] += 1;
    }
    EXPECT_LT(snapshot_nodes(snapshots), count * 16);
    for (std::size_t index = 0; index < count; ++index)
    {
        EXPECT_EQ(snapshots[index].at(index), index);
        if (index != 0) EXPECT_EQ(snapshots[index].at(index - 1), index);
    }
}

TEST(lifetime_state_map, deserialized_entries_rebuild_independent_snapshots)
{
    map_type original;
    original.deserialize_interface() = {{2, 12}, {8, 18}};
    map_type copy = original;
    EXPECT_EQ(original.at(2), 12);
    copy[2] = 42;
    EXPECT_EQ(original.at(2), 12);
    EXPECT_EQ(copy.at(2), 42);
}
