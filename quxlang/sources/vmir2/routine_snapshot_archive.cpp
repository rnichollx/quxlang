// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/routine_snapshot_archive.hpp>

#include <map>
#include <stdexcept>
#include <utility>

namespace quxlang::vmir2
{
    /** Assigns deterministic archive indices to shared metadata in first-use order. */
    template < typename Value >
    class lifetime_payload_archive
    {
      public:
        /** Records one immutable optional payload without repeating its variable-size contents. */
        auto append(shared_lifetime_optional< Value > const& value, std::vector< Value >& output) -> std::optional< std::size_t >
        {
            if (!value.has_value()) return std::nullopt;
            std::pair< typename std::map< Value const*, std::size_t >::iterator, bool > entry = indices_.emplace(&*value, output.size());
            if (entry.second) output.push_back(*value);
            return entry.first->second;
        }

      private:
        std::map< Value const*, std::size_t > indices_;
    };
}

auto quxlang::vmir2::archive_routine_snapshots(functanoid_routine3 const& routine) -> routine_snapshot_archive
{
    routine_snapshot_archive archive{
        .coroutine = routine.coroutine,
        .is_noexcept = routine.is_noexcept,
        .local_types = routine.local_types,
        .parameters = routine.parameters,
        .blocks = routine.blocks,
        .block_names = routine.block_names,
        .non_trivial_dtors = routine.non_trivial_dtors,
        .asynchronous_cleanup_slots = routine.asynchronous_cleanup_slots,
        .static_snapshots = routine.static_snapshots,
        .retained_bindings = routine.retained_bindings,
    };
    lifetime_snapshot_graph< state_map > graph;
    for (executable_block& block : archive.blocks)
    {
        archive.roots.push_back(graph.append(block.entry_state.snapshot_root()));
        block.entry_state.clear();
        for (vm_instruction& instruction : block.instructions)
        {
            if (instruction.type_is< lifetime_transfer >())
            {
                lifetime_transfer& transfer = instruction.as< lifetime_transfer >();
                archive.instruction_roots.push_back(graph.append(transfer.state.snapshot_root()));
                transfer.state.clear();
            }
            else if (instruction.type_is< lifetime_import >())
            {
                lifetime_import& imported = instruction.as< lifetime_import >();
                state_map single;
                single.emplace(imported.slot, std::move(imported.state));
                archive.instruction_roots.push_back(graph.append(single.snapshot_root()));
                imported.state = {};
            }
        }
    }
    lifetime_payload_archive< dtor_spec > destructors;
    lifetime_payload_archive< std::vector< struct_init_delegate > > struct_delegates;
    lifetime_payload_archive< invocation_args > array_delegates;
    using element_map = lifetime_state_map< local_index, bool >;
    lifetime_snapshot_graph< element_map > elements;
    archive.states = archive_lifetime_snapshots< state_map, archived_lifetime_slot >(graph, [&](slot_state const& slot)
    {
        archived_lifetime_slot result{
            .scalar = slot,
            .destructor = destructors.append(slot.nontrivial_dtor, archive.destructors),
            .struct_delegates = struct_delegates.append(slot.struct_delegates, archive.struct_delegates),
            .array_delegates = array_delegates.append(slot.array_delegates, archive.array_delegates),
            .array_elements = elements.append(slot.array_element_delegates.serialize_interface().snapshot_root()),
        };
        result.scalar.nontrivial_dtor.reset();
        result.scalar.struct_delegates.reset();
        result.scalar.array_delegates.reset();
        result.scalar.array_element_delegates.clear();
        return result;
    });
    archive.array_elements = archive_lifetime_snapshots< element_map, bool >(elements, [](bool value) { return value; });
    return archive;
}

auto quxlang::vmir2::restore_routine_snapshots(routine_snapshot_archive archive) -> functanoid_routine3
{
    if (archive.roots.size() != archive.blocks.size())
    {
        throw std::invalid_argument("Serialized routine lifetime roots do not match the block count");
    }
    using element_map = lifetime_state_map< local_index, bool >;
    std::vector< element_map::snapshot > elements = restore_lifetime_snapshots< element_map >(archive.array_elements, [](bool value) { return value; });
    std::vector< shared_lifetime_optional< dtor_spec > > destructors;
    std::vector< shared_lifetime_optional< std::vector< struct_init_delegate > > > struct_delegates;
    std::vector< shared_lifetime_optional< invocation_args > > array_delegates;
    for (dtor_spec& payload : archive.destructors) destructors.emplace_back(std::move(payload));
    for (std::vector< struct_init_delegate >& payload : archive.struct_delegates) struct_delegates.emplace_back(std::move(payload));
    for (invocation_args& payload : archive.array_delegates) array_delegates.emplace_back(std::move(payload));
    std::vector< state_map::snapshot > states = restore_lifetime_snapshots< state_map >(archive.states, [&](archived_lifetime_slot const& slot)
    {
        slot_state result = slot.scalar;
        if (slot.destructor.has_value()) result.nontrivial_dtor = destructors.at(*slot.destructor);
        if (slot.struct_delegates.has_value()) result.struct_delegates = struct_delegates.at(*slot.struct_delegates);
        if (slot.array_delegates.has_value()) result.array_delegates = array_delegates.at(*slot.array_delegates);
        if (slot.array_elements.has_value()) result.array_element_delegates.deserialize_interface() = element_map::from_snapshot(elements.at(*slot.array_elements));
        return result;
    });
    std::size_t instruction_root = 0;
    for (std::size_t index = 0; index < archive.blocks.size(); ++index)
    {
        if (!archive.blocks[index].entry_state.empty())
        {
            throw std::invalid_argument("Serialized routine blocks must reference their shared lifetime graph");
        }
        if (archive.roots[index].has_value()) archive.blocks[index].entry_state = state_map::from_snapshot(states.at(*archive.roots[index]));
        for (vm_instruction& instruction : archive.blocks[index].instructions)
        {
            if (!instruction.type_is< lifetime_transfer >() && !instruction.type_is< lifetime_import >()) continue;
            std::optional< std::size_t > root = archive.instruction_roots.at(instruction_root++);
            state_map restored;
            if (root.has_value()) restored = state_map::from_snapshot(states.at(*root));
            if (instruction.type_is< lifetime_transfer >())
            {
                lifetime_transfer& transfer = instruction.as< lifetime_transfer >();
                if (!transfer.state.empty()) throw std::invalid_argument("Serialized lifetime transfers must reference their shared graph");
                transfer.state = std::move(restored);
            }
            else
            {
                lifetime_import& imported = instruction.as< lifetime_import >();
                if (imported.state != slot_state{} || restored.size() != 1 || !restored.contains(imported.slot))
                    throw std::invalid_argument("Serialized lifetime imports require one shared slot version");
                imported.state = restored.at(imported.slot);
            }
        }
    }
    if (instruction_root != archive.instruction_roots.size()) throw std::invalid_argument("Serialized routine has unused lifetime instruction roots");
    return functanoid_routine3{
        .coroutine = std::move(archive.coroutine),
        .is_noexcept = archive.is_noexcept,
        .local_types = std::move(archive.local_types),
        .parameters = std::move(archive.parameters),
        .blocks = std::move(archive.blocks),
        .block_names = std::move(archive.block_names),
        .non_trivial_dtors = std::move(archive.non_trivial_dtors),
        .asynchronous_cleanup_slots = std::move(archive.asynchronous_cleanup_slots),
        .static_snapshots = std::move(archive.static_snapshots),
        .retained_bindings = std::move(archive.retained_bindings),
    };
}
