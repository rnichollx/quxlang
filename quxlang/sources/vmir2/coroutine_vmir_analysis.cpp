// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/vmir2/coroutine_vmir_analysis.hpp>

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/vmir2/state_engine.hpp>

#include <quxlang/vmir2/local_operands.hpp>

#include "coroutine_lifetime_snapshots.hpp"
#include <quxlang/vmir2/local_borrow_analysis.hpp>

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>

auto quxlang::vmir2::coroutine_analysis_input(functanoid_routine3 const& routine) -> coroutine_vmir_analysis
{
    if (!routine.coroutine.has_value())
    {
        throw compiler_bug("Coroutine frame analysis requires logical coroutine VMIR");
    }
    coroutine_body const& coroutine = *routine.coroutine;
    coroutine_vmir_analysis output;
    output.input.nodes.resize(routine.blocks.size());
    output.input.objects.resize(routine.local_types.size());
    for (coroutine_frame_object& object : output.input.objects)
    {
        object.lifetime_complete = true;
        object.requires_stable_address = false;
    }
    auto execution_parameter = [&](local_index index) -> bool
    {
        return index == coroutine.frame || index == coroutine.controller;
    };
    auto record = [&](local_index local, std::size_t node, std::size_t instruction, coroutine_lifetime_event_kind kind)
    {
        std::size_t index = static_cast< std::uint64_t >(local);
        if (index >= output.input.objects.size())
        {
            throw compiler_bug("Coroutine VMIR contains an invalid local index");
        }
        if (!execution_parameter(local))
        {
            output.input.objects[index].events.push_back({.node = node, .instruction_index = instruction, .kind = kind});
        }
    };
    std::vector< std::optional< std::size_t > > suspension_nodes(routine.blocks.size());
    for (std::size_t block = 0; block < routine.blocks.size(); ++block)
    {
        executable_block const& body = routine.blocks[block];
        if (body.terminator.has_value() && body.terminator->type_is< co_suspend >())
        {
            suspension_nodes[block] = output.input.nodes.size();
            output.input.nodes.push_back({.may_suspend = true});
        }
    }
    std::size_t initialization_boundary = output.input.nodes.size();
    output.input.entry_node = initialization_boundary;
    output.input.nodes.push_back({.may_suspend = true});
    if (!routine.blocks.empty()) output.input.nodes.back().successors.push_back(0);

    std::set< local_index > owned_storage;
    auto capture_parameter = [&](routine_parameter const& parameter)
    {
        if (execution_parameter(parameter.local_index)) return;
        owned_storage.insert(parameter.local_index);
        record(parameter.local_index, initialization_boundary, 0, coroutine_lifetime_event_kind::begin);
        output.input.objects.at(static_cast< std::uint64_t >(parameter.local_index)).force_persistent = true;
        if (parameter.type.type_is< nvalue_slot >() || parameter.type.type_is< dvalue_slot >())
        {
            output.borrowed_parameters.emplace(parameter.local_index, parameter.type);
        }
    };
    for (routine_parameter const& parameter : routine.parameters.positional) capture_parameter(parameter);
    for (std::pair< std::string const, routine_parameter > const& parameter : routine.parameters.named) capture_parameter(parameter.second);
    if (coroutine.cancellation_state.has_value())
    {
        owned_storage.insert(*coroutine.cancellation_state);
        record(*coroutine.cancellation_state, initialization_boundary, 0, coroutine_lifetime_event_kind::begin);
        output.input.objects.at(static_cast< std::uint64_t >(*coroutine.cancellation_state)).force_persistent = true;
    }

    std::vector< coroutine_snapshot_observation > observations;
    observations.reserve(routine.blocks.size() * 2);
    for (std::size_t block = 0; block < routine.blocks.size(); ++block)
    {
        executable_block const& body = routine.blocks[block];
        state_map state = body.entry_state;
        routine_parameters replay_parameters;
        codegen_state_engine engine(state, routine.local_types, replay_parameters);
        if (block == 0 && state.empty())
        {
            codegen_state_engine entry_engine(state, routine.local_types, routine.parameters);
            entry_engine.apply_entry();
        }
        observations.push_back({.root = state.snapshot_root(), .node = block, .instruction = 0});
        for (std::size_t position = 0; position < body.instructions.size(); ++position)
        {
            vm_instruction const& instruction = body.instructions[position];
            std::size_t ordinal = position + 1;
            visit_local_operands(instruction, [&](local_index local)
            {
                record(local, block, ordinal, coroutine_lifetime_event_kind::access);
            });
            std::vector< local_index > touched = coroutine_instruction_locals(instruction, state);
            std::vector< bool > allocated;
            allocated.reserve(touched.size());
            for (local_index local : touched)
            {
                state_map::const_iterator prior = state.find(local);
                allocated.push_back(prior != state.end() && (prior->second.storage_valid || prior->second.alive()));
            }
            if (instruction.type_is< lifetime_transfer >())
            {
                observations.push_back({.root = state.snapshot_root(), .node = block, .instruction = ordinal});
                observations.push_back({.root = instruction.as< lifetime_transfer >().state.snapshot_root(), .node = block, .instruction = ordinal});
            }
            engine.apply_validated(instruction);
            if (instruction.type_is< lifetime_import >())
            {
                record(instruction.as< lifetime_import >().slot, block, ordinal, coroutine_lifetime_event_kind::access);
                observations.push_back({.root = state.snapshot_root(), .node = block, .instruction = ordinal});
            }
            for (std::size_t index = 0; index < touched.size(); ++index)
            {
                local_index local = touched[index];
                state_map::const_iterator next = state.find(local);
                bool remains = next != state.end() && (next->second.storage_valid || next->second.alive());
                if (!allocated[index] && remains && !instruction.type_is< lifetime_import >() && !instruction.type_is< lifetime_transfer >() &&
                    !next->second.is_projection && !next->second.delegate_of.has_value() &&
                    !next->second.array_delegate_of_initializer.has_value() && !next->second.storage_origin.has_value() &&
                    !coroutine.cleanup_aliases.contains(local))
                    owned_storage.insert(local);
                if (!allocated[index] && remains) record(local, block, ordinal, coroutine_lifetime_event_kind::begin);
                if (allocated[index] && !remains) record(local, block, ordinal, coroutine_lifetime_event_kind::end);
                if (remains && !routine.local_types.at(local).type.type_is< array_initializer_type >() &&
                    (next->second.is_projection || next->second.delegate_of.has_value() || next->second.array_delegate_of_initializer.has_value()))
                {
                    output.borrowed_locals.insert(local);
                }
                if (remains && next->second.array_delegate_of_initializer.has_value() && next->second.stage == slot_stage::full && !next->second.owns_subobject_lifetime)
                {
                    output.saved_array_element_addresses.insert(local);
                }
            }
            auto expose_address = [&](local_index local)
            {
                if (execution_parameter(local)) return;
                coroutine_frame_object& object = output.input.objects.at(static_cast< std::uint64_t >(local));
                object.requires_stable_address = true;
            };
            if (instruction.type_is< make_reference >()) expose_address(instruction.as< make_reference >().value_index);
            else if (instruction.type_is< make_pointer_to >()) expose_address(instruction.as< make_pointer_to >().of_index);
            else if (instruction.type_is< access_field >()) expose_address(instruction.as< access_field >().base_index);
            else if (instruction.type_is< access_array >()) expose_address(instruction.as< access_array >().base_index);
            else if (instruction.type_is< fusion_storage_ref >()) expose_address(instruction.as< fusion_storage_ref >().subject);
            else if (instruction.type_is< storage_pun >()) expose_address(instruction.as< storage_pun >().from_storage);
            else if (instruction.type_is< storage_init_start >())
            {
                storage_init_start const& start = instruction.as< storage_init_start >();
                expose_address(start.on_storage);
                output.borrowed_locals.insert(start.target_value);
            }
            else if (instruction.type_is< storage_deinit_start >())
            {
                storage_deinit_start const& start = instruction.as< storage_deinit_start >();
                expose_address(start.on_storage);
                output.borrowed_locals.insert(start.target_value);
            }
            else if (instruction.type_is< storage_rebind >())
            {
                storage_rebind const& rebind = instruction.as< storage_rebind >();
                expose_address(rebind.on_storage);
                output.borrowed_locals.insert(rebind.target_value);
            }
        }
        observations.push_back({.root = state.snapshot_root(), .node = block, .instruction = body.instructions.size() + 1});
        if (body.catcher.has_value())
        {
            output.input.nodes[block].successors.push_back(static_cast< std::uint64_t >(body.catcher->handler));
            if (!body.catcher->terminate_before_capture)
                record(body.catcher->exception, block, body.instructions.size() + 1, coroutine_lifetime_event_kind::access);
        }
        if (!body.terminator.has_value()) continue;
        vm_terminator const& terminator = *body.terminator;
        visit_local_operands(terminator, [&](local_index local)
        {
            record(local, block, body.instructions.size() + 1, coroutine_lifetime_event_kind::access);
        });
        auto add_target = [&](block_index target)
        {
            output.input.nodes[block].successors.push_back(static_cast< std::uint64_t >(target));
        };
        if (terminator.type_is< jump >()) add_target(terminator.as< jump >().target);
        else if (terminator.type_is< branch >())
        {
            add_target(terminator.as< branch >().target_true);
            add_target(terminator.as< branch >().target_false);
        }
        else if (terminator.type_is< tablebranch >())
        {
            for (block_index target : terminator.as< tablebranch >().targets) add_target(target);
            add_target(terminator.as< tablebranch >().default_target);
        }
        else if (terminator.type_is< policy_branch >())
        {
            for (block_index target : terminator.as< policy_branch >().targets) add_target(target);
        }
        else if (terminator.type_is< runtime_constexpr >())
        {
            add_target(terminator.as< runtime_constexpr >().target_constexpr);
            add_target(terminator.as< runtime_constexpr >().target_native);
        }
        else if (terminator.type_is< initguard_try_acquire >())
        {
            add_target(terminator.as< initguard_try_acquire >().target_acquired);
            add_target(terminator.as< initguard_try_acquire >().target_already_initialized);
        }
        else if (terminator.type_is< co_suspend >())
        {
            std::size_t continuation_index = static_cast< std::uint64_t >(terminator.as< co_suspend >().continuation);
            if (continuation_index >= coroutine.continuations.size())
            {
                throw compiler_bug("Coroutine suspension references an unknown continuation");
            }
            coroutine_continuation const& continuation = coroutine.continuations[continuation_index];
            std::size_t suspension = *suspension_nodes[block];
            output.input.nodes[block].successors.push_back(suspension);
            coroutine_cfg_node& suspended = output.input.nodes[suspension];
            suspended.successors.push_back(static_cast< std::uint64_t >(continuation.resume));
            if (continuation.cancellation.has_value()) suspended.successors.push_back(static_cast< std::uint64_t >(*continuation.cancellation));
            record(continuation.awaiter, suspension, 0, coroutine_lifetime_event_kind::access);
        }
    }
    coroutine_snapshot_statistics snapshots = record_coroutine_snapshot_lifetimes(routine, observations, output);
    // Cleanup views preserve the allocation established by a parameter capture or owning definition.
    // Only objects whose storage originates outside the local retain an alias address.
    for (local_index local : owned_storage)
        if (!output.borrowed_parameters.contains(local)) output.borrowed_locals.erase(local);
    output.snapshot_nodes = snapshots.nodes;
    output.snapshot_operands = snapshots.metadata_operands;
    local_borrow_analysis borrows = analyze_local_borrows(routine);
    output.alias_work = borrows.operands + borrows.alias_edges + borrows.traversal_visits;
    for (std::size_t index = 0; index < borrows.escaped_storage.size(); ++index)
    {
        if (!borrows.escaped_storage[index] || execution_parameter(local_index(index))) continue;
        output.input.objects[index].lifetime_complete = false;
        output.input.objects[index].requires_stable_address = true;
    }
    for (std::pair< local_index const, local_index > const& alias : coroutine.cleanup_aliases)
    {
        output.borrowed_locals.insert(alias.first);
        coroutine_frame_object& address = output.input.objects.at(static_cast< std::uint64_t >(alias.second));
        address.force_persistent = true;
        address.lifetime_complete = false;
        address.requires_stable_address = true;
    }
    for (local_index retained : coroutine.cleanup_retained)
    {
        if (execution_parameter(retained) || coroutine.cleanup_aliases.contains(retained)) continue;
        coroutine_frame_object& object = output.input.objects.at(static_cast< std::uint64_t >(retained));
        object.force_persistent = true;
        object.lifetime_complete = false;
        object.requires_stable_address = true;
        if (output.borrowed_locals.contains(retained) && !output.borrowed_parameters.contains(retained))
            output.saved_cleanup_alias_addresses.insert(retained);
    }
    for (local_index borrowed : output.borrowed_locals)
    {
        if (output.borrowed_parameters.contains(borrowed) || output.saved_array_element_addresses.contains(borrowed) ||
            output.saved_cleanup_alias_addresses.contains(borrowed)) continue;
        coroutine_frame_object& object = output.input.objects.at(static_cast< std::uint64_t >(borrowed));
        object.events.clear();
        object.lifetime_complete = true;
        object.force_persistent = false;
    }
    return output;
}

auto quxlang::vmir2::coroutine_instruction_locals(vm_instruction const& instruction) -> std::vector< local_index >
{
    std::vector< local_index > result;
    visit_local_operands(instruction, [&](local_index local) { result.push_back(local); });
    if (instruction.type_is< lifetime_import >()) result.push_back(instruction.as< lifetime_import >().slot);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

auto quxlang::vmir2::coroutine_instruction_locals(vm_instruction const& instruction, state_map const& state) -> std::vector< local_index >
{
    std::vector< local_index > result = coroutine_instruction_locals(instruction);
    std::optional< local_index > retired;
    if (instruction.type_is< array_init_finish >()) retired = instruction.as< array_init_finish >().initializer;
    else if (instruction.type_is< struct_init_finish >()) retired = instruction.as< struct_init_finish >().on_value;
    else if (instruction.type_is< end_lifetime >()) retired = instruction.as< end_lifetime >().of;
    if (retired.has_value() && state.contains(*retired))
    {
        std::vector< local_index > bindings = codegen_state_engine::retained_storage_bindings(state, *retired);
        result.insert(result.end(), bindings.begin(), bindings.end());
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
    }
    return result;
}
