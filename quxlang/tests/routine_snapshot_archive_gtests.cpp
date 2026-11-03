// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/routine_snapshot_archive.hpp>
#include <quxlang/queries/querygraph_traits.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <iterator>
#include <vector>

namespace quxlang::vmir2
{
    TEST(routine_snapshot_archive, binary_roundtrip_preserves_shared_lifetimes_and_delegate_payloads)
    {
        constexpr std::size_t locals = 256;
        constexpr std::size_t blocks = 512;
        state_map state;
        for (std::size_t index = 0; index < locals; ++index)
        {
            state.emplace(local_index(index), slot_state{.stage = slot_stage::full, .storage_valid = true});
        }
        std::vector< struct_init_delegate > delegates;
        for (std::size_t index = 1; index < locals; ++index)
        {
            delegates.push_back({.selector = struct_init_field_selector{.field_ordinal = index}, .value = local_index(index)});
        }
        state.at(local_index(0)).struct_delegates = std::move(delegates);
        functanoid_routine3 routine;
        for (std::size_t index = 0; index < blocks; ++index)
        {
            routine.blocks.push_back({.entry_state = state});
            state.at(local_index(index % locals)).destruction_started = index % 2 != 0;
        }
        routine_snapshot_archive archive = archive_routine_snapshots(routine);
        EXPECT_EQ(archive.struct_delegates.size(), 1U);
        EXPECT_LT(archive.states.size(), 32 * (locals + blocks));
        std::vector< std::byte > bytes;
        rpnx::serial4::serialize_iter(routine, std::back_inserter(bytes));
        std::vector< std::byte > json;
        rpnx::serial4::json_serialize_iter(routine, std::back_inserter(json));
        EXPECT_FALSE(json.empty());
        functanoid_routine3 restored;
        EXPECT_EQ(rpnx::serial4::deserialize_iter(restored, bytes.begin(), bytes.end()), bytes.end());
        EXPECT_EQ(restored, routine);
        EXPECT_EQ(&*std::as_const(restored).blocks.front().entry_state.at(local_index(0)).struct_delegates,
                  &*std::as_const(restored).blocks.back().entry_state.at(local_index(0)).struct_delegates);
        routine_snapshot_archive second_archive = archive_routine_snapshots(restored);
        EXPECT_EQ(second_archive.states.size(), archive.states.size());
        std::vector< std::byte > repeated;
        rpnx::serial4::serialize_iter(restored, std::back_inserter(repeated));
        EXPECT_EQ(bytes, repeated);
        RecordProperty("snapshot_nodes", archive.states.size());
        RecordProperty("binary_bytes", bytes.size());
        RecordProperty("json_bytes", json.size());
    }

    TEST(routine_snapshot_archive, ownership_transfer_preserves_shared_snapshot_identity)
    {
        state_map state;
        for (std::size_t index = 0; index < 128; ++index) state[local_index(index)] = {.stage = slot_stage::full, .storage_valid = true};
        functanoid_routine3 routine;
        routine.asynchronous_cleanup_slots.insert(local_index(7));
        for (std::size_t index = 0; index < 128; ++index)
        {
            routine.blocks.push_back({.entry_state = state, .instructions = {
                lifetime_transfer{.state = state}, lifetime_import{.slot = local_index(7), .state = std::as_const(state).at(local_index(7))}}});
            state[local_index(index)].destruction_started = true;
        }
        routine_snapshot_archive archive = archive_routine_snapshots(routine);
        EXPECT_LT(archive.states.size(), 4096);
        EXPECT_EQ(archive.instruction_roots.size(), 256);
        std::vector< std::byte > bytes;
        rpnx::serial4::serialize_iter(routine, std::back_inserter(bytes));
        functanoid_routine3 restored;
        EXPECT_EQ(rpnx::serial4::deserialize_iter(restored, bytes.begin(), bytes.end()), bytes.end());
        EXPECT_EQ(restored, routine);
        for (executable_block const& block : restored.blocks)
        {
            EXPECT_EQ(block.entry_state.snapshot_root(), block.instructions.front().as< lifetime_transfer >().state.snapshot_root());
        }
    }
}
