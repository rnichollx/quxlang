// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/lifetime_state_difference.hpp>
#include <quxlang/vmir2/lifetime_transition_graph.hpp>
#include <quxlang/vmir2/lifetime_cleanup_program.hpp>

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

    TEST(LifetimeCleanupProgram, CursorPreservesPlannerOrderingForDistinctTargets)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(32, {.type = int_type{.bits = 32, .has_sign = false}});
        state_map source;
        for (std::size_t index = 0; index < 32; ++index)
        {
            slot_state slot{.stage = slot_stage::full, .storage_valid = true};
            if (index % 3 == 0) slot.nontrivial_dtor = dtor_spec{.func = submember{.of = absolute_module_reference{"TEST"}, .name = "DESTROY"}};
            source[local_index(index)] = std::move(slot);
        }
        lifetime_cleanup_program program(routine);
        lifetime_transition_graph planner(routine);
        for (std::size_t selection = 0; selection < 32; ++selection)
        {
            state_map target = source;
            for (std::size_t index = 0; index < 32; ++index)
            {
                if (((index * 7 + selection * 11) % 13) < 6) target.erase(local_index(index));
            }
            std::vector< lifetime_action > expected = planner.flatten(planner.select(source, target, lifetime_transition_kind::control_flow));
            lifetime_cleanup_program::cursor cursor{.transition_index = program.append(source, target, lifetime_transition_kind::control_flow)};
            std::size_t index = 0;
            while (std::optional< lifetime_cleanup_program::selected_action > action = program.next(cursor))
            {
                ASSERT_LT(index, expected.size());
                EXPECT_EQ(action->action.slot, expected[index].slot);
                EXPECT_EQ(action->action.kind, expected[index].kind);
                ++index;
            }
            EXPECT_EQ(index, expected.size());
            EXPECT_TRUE(cursor.completed);
            EXPECT_FALSE(program.next(cursor).has_value());
        }
    }

    TEST(LifetimeCleanupProgram, PrefixTargetsKeepBoundedMetadata)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(128, {.type = int_type{.bits = 32, .has_sign = false}});
        state_map current;
        std::vector< state_map > prefixes;
        for (std::size_t index = 0; index < 128; ++index)
        {
            prefixes.push_back(current);
            current[local_index(index)] = {.stage = slot_stage::full, .storage_valid = true};
        }
        lifetime_cleanup_program program(routine);
        for (std::size_t index = 0; index < prefixes.size(); ++index)
        {
            lifetime_cleanup_program::cursor cursor{.transition_index = program.append(current, prefixes[index], lifetime_transition_kind::control_flow)};
            std::size_t remaining = 128;
            while (std::optional< lifetime_cleanup_program::selected_action > action = program.next(cursor))
            {
                EXPECT_EQ(action->action.kind, lifetime_action_kind::end_lifetime);
                EXPECT_EQ(action->action.slot, local_index(--remaining));
            }
            EXPECT_EQ(remaining, index);
        }
        EXPECT_EQ(program.transitions().size(), 128);
        EXPECT_LT(program.graph().nodes().size(), 1024);
        RecordProperty("cleanup_program_snapshot_nodes", static_cast<int>(program.graph().nodes().size()));
    }

    TEST(LifetimeTransitionGraph, ExhaustedExpansionRetainsOnlyCompletedProofs)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(128, {.type = int_type{.bits = 32, .has_sign = false}});
        state_map source;
        for (std::size_t index = 0; index < 128; ++index) source[local_index(index)] = {.stage = slot_stage::full, .storage_valid = true};
        state_map target;
        lifetime_transition_graph graph(routine);
        std::uint64_t remaining = 37;
        EXPECT_FALSE(graph.select_bounded(source, target, lifetime_transition_kind::control_flow, remaining).has_value());
        EXPECT_EQ(remaining, 0);
        EXPECT_LE(graph.node_count(), 37);
        std::vector< lifetime_action > completed = graph.flatten(graph.select(source, target, lifetime_transition_kind::control_flow));
        ASSERT_EQ(completed.size(), 128);
        for (std::size_t index = 0; index < completed.size(); ++index) EXPECT_EQ(completed[index].slot, local_index(127 - index));
    }

    TEST(LifetimeCleanupProgram, ParkedReservationRetiresWithoutDestructor)
    {
        functanoid_routine3 routine;
        routine.local_types.push_back({.type = int_type{.bits = 32, .has_sign = false}});
        routine.coroutine.emplace();
        routine.coroutine->cleanup_retained.insert(local_index(0));
        state_map source;
        source.emplace(local_index(0), slot_state{.storage_valid = true});
        lifetime_cleanup_program program(routine);
        lifetime_cleanup_program::cursor cursor{.transition_index = program.append(source, {}, lifetime_transition_kind::control_flow)};
        std::optional< lifetime_cleanup_program::selected_action > action = program.next(cursor);
        ASSERT_TRUE(action.has_value());
        EXPECT_EQ(action->action.kind, lifetime_action_kind::end_lifetime);
        EXPECT_EQ(action->action.slot, local_index(0));
        EXPECT_FALSE(program.next(cursor).has_value());
        std::vector< lifetime_cleanup_program::encoded_node > encoded = program.encode();
        ASSERT_EQ(encoded.size(), 2U);
        EXPECT_EQ(encoded[1][static_cast< std::size_t >(lifetime_cleanup_program::column::lifetime_flags)], 2U);
        EXPECT_EQ(encoded[1][static_cast< std::size_t >(lifetime_cleanup_program::column::control_flow_actions)], 32U);
    }

    TEST(LifetimeCleanupProgram, TargetOnlyVersionsBecomeSourcesOnDemand)
    {
        functanoid_routine3 routine;
        routine.local_types.resize(2, {.type = int_type{.bits = 32, .has_sign = false}});
        state_map source;
        source.emplace(local_index(0), slot_state{.stage = slot_stage::full, .storage_valid = true});
        state_map target;
        target.emplace(local_index(1), slot_state{.stage = slot_stage::full, .storage_valid = true});
        lifetime_cleanup_program program(routine);
        std::size_t route = program.append(source, target, lifetime_transition_kind::control_flow);
        std::size_t source_root = *program.transitions().at(route).source;
        std::size_t target_root = *program.transitions().at(route).target;
        EXPECT_TRUE(program.source_nodes().at(source_root));
        EXPECT_FALSE(program.source_nodes().at(target_root));
        program.append(target, source, lifetime_transition_kind::control_flow);
        EXPECT_TRUE(program.source_nodes().at(target_root));
        EXPECT_EQ(program.graph().nodes().size(), 2U);
    }
}
