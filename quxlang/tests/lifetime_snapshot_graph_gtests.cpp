// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/lifetime_snapshot_graph.hpp>
#include <quxlang/vmir2/lifetime_snapshot_archive.hpp>
#include <quxlang/vmir2/lifetime_state_map.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace
{
    using test_state = quxlang::vmir2::lifetime_state_map< std::uint64_t, std::uint64_t >;
    using snapshot_graph = quxlang::vmir2::lifetime_snapshot_graph< test_state >;
    using quxlang::vmir2::lifetime_snapshot_range;
}

TEST(lifetime_snapshot_graph, shared_subtrees_receive_all_root_obligations)
{
    test_state first{{1, 10}, {2, 20}, {3, 30}};
    test_state second = first;
    second[2] = 21;
    snapshot_graph graph;
    std::size_t first_root = *graph.append(first.snapshot_root());
    std::size_t second_root = *graph.append(second.snapshot_root());
    std::vector< lifetime_snapshot_range > ranges(graph.nodes().size());
    ranges[first_root].include({.component = 2, .node = 1, .instruction = 4});
    ranges[second_root].include({.component = 7, .node = 3, .instruction = 8});
    graph.propagate(ranges);
    std::size_t leaves = 0;
    for (std::size_t index = 0; index < graph.nodes().size(); ++index)
    {
        snapshot_graph::node const& node = graph.nodes()[index];
        if (!node.source->value) continue;
        ++leaves;
        ASSERT_TRUE(ranges[index].first.has_value());
        ASSERT_TRUE(ranges[index].last.has_value());
        if (node.source->value->first == 2)
        {
            std::size_t component = node.source->value->second == 20 ? 2 : 7;
            EXPECT_EQ(ranges[index].first->component, component);
            EXPECT_EQ(ranges[index].last->component, component);
        }
        else
        {
            EXPECT_EQ(ranges[index].first->component, 2);
            EXPECT_EQ(ranges[index].last->component, 7);
        }
    }
    EXPECT_EQ(leaves, 4);
}

TEST(lifetime_snapshot_graph, many_large_snapshots_share_bounded_metadata)
{
    constexpr std::size_t count = 4096;
    test_state state;
    for (std::size_t index = 0; index < count; ++index) state[index] = index;
    snapshot_graph graph;
    std::vector< std::size_t > roots;
    for (std::size_t index = 0; index < count; ++index)
    {
        roots.push_back(*graph.append(state.snapshot_root()));
        state[index] = index + count;
    }
    EXPECT_LT(graph.nodes().size(), count * 16);
    std::vector< lifetime_snapshot_range > ranges(graph.nodes().size());
    for (std::size_t index = 0; index < roots.size(); ++index)
    {
        ranges[roots[index]].include({.component = index, .node = index, .instruction = 0});
    }
    graph.propagate(ranges);
    for (std::size_t index = 0; index < graph.nodes().size(); ++index)
    {
        snapshot_graph::node const& node = graph.nodes()[index];
        if (!node.source->value) continue;
        std::size_t key = node.source->value->first;
        if (node.source->value->second < count)
        {
            EXPECT_EQ(ranges[index].first->component, 0);
            EXPECT_EQ(ranges[index].last->component, key);
        }
        else
        {
            EXPECT_EQ(ranges[index].first->component, key + 1);
            EXPECT_EQ(ranges[index].last->component, count - 1);
        }
    }
}

TEST(lifetime_snapshot_graph, shared_transform_filters_and_renames_each_version_once)
{
    constexpr std::size_t count = 1024;
    test_state state;
    for (std::size_t index = 0; index < count; ++index) state[index] = index;
    quxlang::vmir2::lifetime_snapshot_transform< test_state > transform;
    snapshot_graph output;
    std::size_t visits = 0;
    for (std::size_t index = 0; index < count; ++index)
    {
        test_state converted = test_state::from_snapshot(transform.apply(state.snapshot_root(),
            [&](test_state::value_type const& value) -> std::optional< test_state::value_type >
            {
                ++visits;
                if (value.first % 3 != 0) return std::nullopt;
                return test_state::value_type(count + value.first / 3, value.second);
            }));
        EXPECT_EQ(converted.size(), (count + 2) / 3);
        EXPECT_TRUE(converted.contains(count));
        output.append(converted.snapshot_root());
        state[index] += count;
    }
    EXPECT_LT(visits, 2 * count);
    EXPECT_LT(output.nodes().size(), 64 * count);
}

TEST(lifetime_snapshot_graph, archive_restores_shared_versions_and_rejects_forward_edges)
{
    test_state state;
    for (std::size_t index = 0; index < 1024; ++index) state[index] = index;
    snapshot_graph graph;
    std::size_t first = *graph.append(state.snapshot_root());
    state[7] = 99;
    std::size_t second = *graph.append(state.snapshot_root());
    auto records = quxlang::vmir2::archive_lifetime_snapshots< test_state, std::uint64_t >(graph, [](std::uint64_t value) { return value; });
    auto nodes = quxlang::vmir2::restore_lifetime_snapshots< test_state >(records, [](std::uint64_t value) { return value; });
    test_state restored_first = test_state::from_snapshot(nodes.at(first));
    test_state restored_second = test_state::from_snapshot(nodes.at(second));
    EXPECT_EQ(std::as_const(restored_first).at(7), 7U);
    EXPECT_EQ(std::as_const(restored_second).at(7), 99U);
    EXPECT_EQ(restored_second, state);
    snapshot_graph restored_graph;
    restored_graph.append(restored_first.snapshot_root());
    restored_graph.append(restored_second.snapshot_root());
    EXPECT_EQ(restored_graph.nodes().size(), graph.nodes().size());
    records.back().left = records.size();
    EXPECT_THROW(quxlang::vmir2::restore_lifetime_snapshots< test_state >(records, [](std::uint64_t value) { return value; }), std::invalid_argument);
}
