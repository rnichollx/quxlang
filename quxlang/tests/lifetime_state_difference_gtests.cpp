// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/lifetime_state_difference.hpp>
#include <quxlang/vmir2/lifetime_transition_graph.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace quxlang::vmir2
{
    TEST(LifetimeStateDifference, SharedSnapshotsVisitOnlyChangedPaths)
    {
        quxlang::vmir2::lifetime_state_map< std::uint64_t, std::uint64_t > source;
        for (std::uint64_t index = 0; index < 4096; ++index) source.emplace(index, index);
        quxlang::vmir2::lifetime_state_map< std::uint64_t, std::uint64_t > target = source;
        target.at(117) = 42;
        target.erase(2048);
        target.emplace(8192, 1);
        std::vector< std::uint64_t > keys;
        std::size_t work = quxlang::vmir2::visit_lifetime_state_difference(source, target,
            [&](std::pair< std::uint64_t const, std::uint64_t > const& entry, std::uint64_t const* after)
            {
                keys.push_back(entry.first);
                if (entry.first == 117) ASSERT_EQ(*after, 42U);
                else ASSERT_EQ(after, nullptr);
            });
        EXPECT_EQ(keys, (std::vector< std::uint64_t >{2048, 117}));
        EXPECT_LT(work, 128U);
    }

    TEST(LifetimeStateDifference, IdenticalSnapshotRequiresOneComparison)
    {
        quxlang::vmir2::lifetime_state_map< std::uint64_t, std::uint64_t > source;
        for (std::uint64_t index = 0; index < 1024; ++index) source.emplace(index, index);
        std::size_t calls = 0;
        std::size_t work = quxlang::vmir2::visit_lifetime_state_difference(source, source,
            [&](std::pair< std::uint64_t const, std::uint64_t > const&, std::uint64_t const*) { ++calls; });
        EXPECT_EQ(work, 1U);
        EXPECT_EQ(calls, 0U);
    }

    TEST(LifetimeStateDifference, DifferentlyShapedTreesMatchKeysInDescendingOrder)
    {
        std::vector< std::uint64_t > keys{0, 1, 2, 7, 16, 33, 127, 255};
        for (std::uint64_t selection = 0; selection < 256; ++selection)
        {
            quxlang::vmir2::lifetime_state_map< std::uint64_t, std::uint64_t > source;
            quxlang::vmir2::lifetime_state_map< std::uint64_t, std::uint64_t > target;
            std::vector< std::uint64_t > expected;
            for (std::size_t index = 0; index < keys.size(); ++index)
            {
                if ((selection & (std::uint64_t(1) << index)) != 0)
                {
                    source.emplace(keys[index], keys[index]);
                    expected.insert(expected.begin(), keys[index]);
                }
                if (((selection + 51) & (std::uint64_t(1) << index)) != 0) target.emplace(keys[index], keys[index] + 1);
            }
            std::vector< std::uint64_t > visited;
            quxlang::vmir2::visit_lifetime_state_difference(source, target,
                [&](std::pair< std::uint64_t const, std::uint64_t > const& entry, std::uint64_t const* after)
                {
                    visited.push_back(entry.first);
                    if (target.contains(entry.first))
                    {
                        ASSERT_NE(after, nullptr);
                        EXPECT_EQ(*after, entry.second + 1);
                    }
                    else EXPECT_EQ(after, nullptr);
                });
            EXPECT_EQ(visited, expected) << selection;
        }
    }

    TEST(LifetimeTransitionGraph, RepeatedExitsShareUnchangedCleanupSequences)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(4096, {.type = int_type{.bits = 32, .has_sign = false}});
        state_map current;
        for (std::size_t index = 0; index < 4096; ++index) current[local_index(index)] = {.stage = slot_stage::full, .storage_valid = true};
        lifetime_transition_graph graph(routine);
        state_map target;
        std::set< lifetime_transition_graph::sequence > roots;
        for (std::size_t index = 0; index < 512; ++index)
        {
            current[local_index(index)].stage = slot_stage::dead;
            current[local_index(index)].storage_valid = false;
            lifetime_transition_graph::plan plan = graph.select(current, target, lifetime_transition_kind::exceptional_exit);
            EXPECT_EQ(plan.delegates, 0);
            roots.insert(plan.ordinary);
        }
        EXPECT_EQ(roots.size(), 512);
        EXPECT_LT(graph.snapshot_visits(), 16000);
        EXPECT_LT(graph.node_count(), 16000);
        RecordProperty("cleanup_snapshot_visits", static_cast<int>(graph.snapshot_visits()));
        RecordProperty("cleanup_sequence_nodes", static_cast<int>(graph.node_count()));
    }

    TEST(LifetimeTransitionGraph, SparseTransitionsSkipSharedSourceSubtrees)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(4096, {.type = int_type{.bits = 32, .has_sign = false}});
        state_map source;
        for (std::size_t index = 0; index < 4096; ++index) source[local_index(index)] = {.stage = slot_stage::full, .storage_valid = true};
        state_map target = source;
        target.erase(local_index(13));
        target.erase(local_index(2049));
        lifetime_transition_graph graph(routine);
        std::vector< lifetime_action > actions = graph.flatten(graph.select(source, target, lifetime_transition_kind::control_flow));
        ASSERT_EQ(actions.size(), 2);
        EXPECT_EQ(actions[0].slot, local_index(2049));
        EXPECT_EQ(actions[1].slot, local_index(13));
        EXPECT_LT(graph.snapshot_visits(), 32);
    }

    TEST(LifetimeTransitionGraph, AncestorLifetimeChangesInvalidateDelegateProofs)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(4, {.type = int_type{.bits = 32, .has_sign = false}});
        state_map source;
        struct_init_field_selector field{.field_ordinal = 0};
        source[local_index(1)] = {.stage = slot_stage::partial, .storage_valid = true};
        source[local_index(2)] = {.stage = slot_stage::partial, .storage_valid = true, .delegate_of = local_index(1), .struct_delegate_selector = field};
        source[local_index(3)] = {.stage = slot_stage::full, .storage_valid = true,
            .nontrivial_dtor = dtor_spec{.func = submember{.of = absolute_module_reference{"TEST"}, .name = "DESTROY"}},
            .delegate_of = local_index(2), .struct_delegate_selector = field};
        state_map target;
        target[local_index(1)] = {.stage = slot_stage::full, .storage_valid = true};
        lifetime_transition_graph graph(routine);
        EXPECT_TRUE(graph.flatten(graph.select(source, target, lifetime_transition_kind::control_flow)).empty());
        source[local_index(2)].owns_subobject_lifetime = true;
        std::vector< lifetime_action > actions = graph.flatten(graph.select(source, target, lifetime_transition_kind::control_flow));
        ASSERT_EQ(actions.size(), 2);
        EXPECT_EQ(actions.front().kind, lifetime_action_kind::destroy_object);
        EXPECT_EQ(actions.front().slot, local_index(3));
    }

}
