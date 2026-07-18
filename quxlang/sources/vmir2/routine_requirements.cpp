// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>
#include <quxlang/vmir2/state_engine.hpp>

#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    auto concrete_functanoid_from_symbol(quxlang::type_symbol const& symbol) -> std::optional< quxlang::type_symbol >
    {
        if (symbol.type_is< quxlang::instanciation_reference >())
        {
            return symbol;
        }

        if (symbol.type_is< quxlang::temploid_reference >())
        {
            throw quxlang::compiler_bug("VMIR2 routine requirements encountered an uninstantiated overload reference: " + quxlang::to_string(symbol));
        }

        return std::nullopt;
    }

    void add_functanoid(std::set< quxlang::type_symbol >& result, quxlang::type_symbol const& symbol)
    {
        auto concrete = concrete_functanoid_from_symbol(symbol);
        if (!concrete.has_value())
        {
            throw quxlang::compiler_bug("VMIR2 routine references a non-functanoid symbol: " + quxlang::to_string(symbol));
        }

        result.insert(std::move(*concrete));
    }

    void add_optional_functanoid(std::set< quxlang::type_symbol >& result, quxlang::type_symbol const& symbol)
    {
        auto concrete = concrete_functanoid_from_symbol(symbol);
        if (concrete.has_value())
        {
            result.insert(std::move(*concrete));
        }
    }

    auto symbol_is_localdata_root(quxlang::type_symbol const& symbol) -> bool
    {
        return symbol.type_is< quxlang::static_local_ref >() || symbol.type_is< quxlang::static_snapshot_ref >();
    }

    auto symbol_is_functanoid_reference(quxlang::type_symbol const& symbol) -> bool
    {
        return symbol.type_is< quxlang::instanciation_reference >() || symbol.type_is< quxlang::temploid_reference >();
    }

    void add_antestatal_global(std::set< quxlang::type_symbol >& result, quxlang::type_symbol const& symbol)
    {
        if (symbol_is_localdata_root(symbol) || symbol_is_functanoid_reference(symbol))
        {
            return;
        }

        result.insert(symbol);
    }

    void add_type_and_components(std::set< quxlang::type_symbol >& result, quxlang::type_symbol type)
    {
        std::vector< quxlang::type_symbol > pending{std::move(type)};
        while (!pending.empty())
        {
            quxlang::type_symbol current = std::move(pending.back());
            pending.pop_back();

            if (!result.insert(current).second)
            {
                continue;
            }

            if (current.type_is< quxlang::nvalue_slot >())
            {
                pending.push_back(current.get_as< quxlang::nvalue_slot >().target);
            }
            else if (current.type_is< quxlang::dvalue_slot >())
            {
                pending.push_back(current.get_as< quxlang::dvalue_slot >().target);
            }
            else if (current.type_is< quxlang::ptrref_type >())
            {
                pending.push_back(current.get_as< quxlang::ptrref_type >().target);
            }
            else if (current.type_is< quxlang::attached_type_reference >())
            {
                pending.push_back(current.get_as< quxlang::attached_type_reference >().carrying_type);
            }
            else if (current.type_is< quxlang::array_type >())
            {
                pending.push_back(current.get_as< quxlang::array_type >().element_type);
            }
            else if (auto atomic_value_type = quxlang::atomic_type_argument(current); atomic_value_type.has_value())
            {
                pending.push_back(std::move(*atomic_value_type));
            }
            else if (current.type_is< quxlang::storage >())
            {
                for (quxlang::type_symbol const& storable_type : current.get_as< quxlang::storage >().storable_types)
                {
                    pending.push_back(storable_type);
                }
            }
        }
    }

    auto type_might_have_layout(quxlang::type_symbol const& type) -> bool
    {
        if (quxlang::is_atomic_type(type))
        {
            return false;
        }

        return type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >() ||
            (type.type_is< quxlang::builtin_symbol >() && quxlang::is_builtin_enum_name(type.get_as< quxlang::builtin_symbol >().name));
    }

    void add_routine_surface_types(std::set< quxlang::type_symbol >& result, quxlang::vmir2::functanoid_routine3 const& routine,
                                   std::set< quxlang::vmir2::local_index > const& locals,
                                   std::set< quxlang::static_snapshot_ref > const& snapshots)
    {
        for (quxlang::vmir2::local_index const local : locals)
        {
            add_type_and_components(result, routine.local_types.at(static_cast< std::uint64_t >(local)).type);
        }
        for (auto const& [_, param] : routine.parameters.named)
        {
            add_type_and_components(result, param.type);
        }
        for (quxlang::vmir2::routine_parameter const& param : routine.parameters.positional)
        {
            add_type_and_components(result, param.type);
        }
        for (quxlang::static_snapshot_ref const& snapshot : snapshots)
        {
            quxlang::vmir2::localdata_entry const& localdata = routine.static_snapshots.at(snapshot);
            add_type_and_components(result, localdata.type);
        }
    }

    auto normalized_antestatal_type(std::optional< quxlang::type_symbol > type) -> std::optional< quxlang::type_symbol >
    {
        if (!type.has_value())
        {
            return std::nullopt;
        }

        if (type->type_is< quxlang::nvalue_slot >())
        {
            return type->get_as< quxlang::nvalue_slot >().target;
        }
        if (type->type_is< quxlang::dvalue_slot >())
        {
            return type->get_as< quxlang::dvalue_slot >().target;
        }
        return type;
    }

    void add_functanoids_from_antestatal_access(std::set< quxlang::type_symbol >& result, quxlang::antestatal_access const& access)
    {
        if (access.type_is< quxlang::antestatal_access_global >())
        {
            add_optional_functanoid(result, access.get_as< quxlang::antestatal_access_global >().symbol);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_field >())
        {
            add_functanoids_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_field >().object);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_array_element >())
        {
            add_functanoids_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_array_element >().array);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_fusion_payload >())
        {
            add_functanoids_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_fusion_payload >().fusion);
        }
    }

    void add_functanoids_from_antestatal_value(std::set< quxlang::type_symbol >& result, quxlang::antestatal_value const& value, std::optional< quxlang::type_symbol > type)
    {
        type = normalized_antestatal_type(std::move(type));

        if (value.type_is< quxlang::antestatal_interface >())
        {
            quxlang::antestatal_interface const& interface_value = value.get_as< quxlang::antestatal_interface >();
            for (std::pair< quxlang::interface_slot_key const, quxlang::type_symbol > const& function : interface_value.functions)
            {
                add_functanoid(result, function.second);
            }
            return;
        }

        if (value.type_is< quxlang::antestatal_ptrref >())
        {
            quxlang::antestatal_access const& access = value.get_as< quxlang::antestatal_ptrref >().target;
            if (type.has_value() && type->type_is< quxlang::ptrref_type >() && type->get_as< quxlang::ptrref_type >().target.type_is< quxlang::procedure_type >())
            {
                if (access.type_is< quxlang::antestatal_access_global >())
                {
                    add_functanoid(result, access.get_as< quxlang::antestatal_access_global >().symbol);
                }
                else
                {
                    add_functanoids_from_antestatal_access(result, access);
                }
                return;
            }

            add_functanoids_from_antestatal_access(result, access);
            return;
        }

        if (value.type_is< quxlang::antestatal_array >())
        {
            std::optional< quxlang::type_symbol > element_type;
            if (type.has_value() && type->type_is< quxlang::array_type >())
            {
                element_type = type->get_as< quxlang::array_type >().element_type;
            }

            for (quxlang::antestatal_value const& element : value.get_as< quxlang::antestatal_array >().elements)
            {
                add_functanoids_from_antestatal_value(result, element, element_type);
            }
            return;
        }

        if (value.type_is< quxlang::antestatal_struct >())
        {
            for (auto const& [_, field_value] : value.get_as< quxlang::antestatal_struct >().fields)
            {
                add_functanoids_from_antestatal_value(result, field_value, std::nullopt);
            }
            return;
        }

        if (value.type_is< quxlang::antestatal_fusion >())
        {
            quxlang::antestatal_fusion const& fusion = value.get_as< quxlang::antestatal_fusion >();
            if (fusion.state.type_is< quxlang::antestatal_fusion_active >())
            {
                quxlang::antestatal_fusion_active const& active = fusion.state.get_as< quxlang::antestatal_fusion_active >();
                if (active.payload.has_value())
                {
                    add_functanoids_from_antestatal_value(result, active.payload.value(), std::nullopt);
                }
            }
        }
    }

    void add_antestatal_globals_from_antestatal_access(std::set< quxlang::type_symbol >& result, quxlang::antestatal_access const& access)
    {
        if (access.type_is< quxlang::antestatal_access_global >())
        {
            add_antestatal_global(result, access.get_as< quxlang::antestatal_access_global >().symbol);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_field >())
        {
            add_antestatal_globals_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_field >().object);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_array_element >())
        {
            add_antestatal_globals_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_array_element >().array);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_fusion_payload >())
        {
            add_antestatal_globals_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_fusion_payload >().fusion);
        }
    }

    void add_antestatal_globals_from_antestatal_value(std::set< quxlang::type_symbol >& result, quxlang::antestatal_value const& value, std::optional< quxlang::type_symbol > type)
    {
        type = normalized_antestatal_type(std::move(type));

        if (value.type_is< quxlang::antestatal_ptrref >())
        {
            quxlang::antestatal_access const& access = value.get_as< quxlang::antestatal_ptrref >().target;
            if (type.has_value() && type->type_is< quxlang::ptrref_type >() && type->get_as< quxlang::ptrref_type >().target.type_is< quxlang::procedure_type >())
            {
                return;
            }

            add_antestatal_globals_from_antestatal_access(result, access);
            return;
        }

        if (value.type_is< quxlang::antestatal_array >())
        {
            std::optional< quxlang::type_symbol > element_type;
            if (type.has_value() && type->type_is< quxlang::array_type >())
            {
                element_type = type->get_as< quxlang::array_type >().element_type;
            }

            for (quxlang::antestatal_value const& element : value.get_as< quxlang::antestatal_array >().elements)
            {
                add_antestatal_globals_from_antestatal_value(result, element, element_type);
            }
            return;
        }

        if (value.type_is< quxlang::antestatal_struct >())
        {
            for (auto const& [_, field_value] : value.get_as< quxlang::antestatal_struct >().fields)
            {
                add_antestatal_globals_from_antestatal_value(result, field_value, std::nullopt);
            }
            return;
        }

        if (value.type_is< quxlang::antestatal_fusion >())
        {
            quxlang::antestatal_fusion const& fusion = value.get_as< quxlang::antestatal_fusion >();
            if (fusion.state.type_is< quxlang::antestatal_fusion_active >())
            {
                quxlang::antestatal_fusion_active const& active = fusion.state.get_as< quxlang::antestatal_fusion_active >();
                if (active.payload.has_value())
                {
                    add_antestatal_globals_from_antestatal_value(result, active.payload.value(), std::nullopt);
                }
            }
        }
    }

    void add_static_snapshots_from_antestatal_access(std::set< quxlang::static_snapshot_ref >& result, quxlang::antestatal_access const& access)
    {
        if (access.type_is< quxlang::antestatal_access_global >())
        {
            quxlang::type_symbol const& symbol = access.get_as< quxlang::antestatal_access_global >().symbol;
            if (symbol.type_is< quxlang::static_snapshot_ref >())
            {
                result.insert(symbol.get_as< quxlang::static_snapshot_ref >());
            }
            return;
        }
        if (access.type_is< quxlang::antestatal_access_field >())
        {
            add_static_snapshots_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_field >().object);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_array_element >())
        {
            add_static_snapshots_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_array_element >().array);
            return;
        }
        if (access.type_is< quxlang::antestatal_access_fusion_payload >())
        {
            add_static_snapshots_from_antestatal_access(result, access.get_as< quxlang::antestatal_access_fusion_payload >().fusion);
        }
    }

    void add_static_snapshots_from_antestatal_value(std::set< quxlang::static_snapshot_ref >& result, quxlang::antestatal_value const& value)
    {
        if (value.type_is< quxlang::antestatal_ptrref >())
        {
            add_static_snapshots_from_antestatal_access(result, value.get_as< quxlang::antestatal_ptrref >().target);
            return;
        }
        if (value.type_is< quxlang::antestatal_array >())
        {
            for (quxlang::antestatal_value const& element : value.get_as< quxlang::antestatal_array >().elements)
            {
                add_static_snapshots_from_antestatal_value(result, element);
            }
            return;
        }
        if (value.type_is< quxlang::antestatal_struct >())
        {
            for (std::pair< std::string const, quxlang::antestatal_value > const& field : value.get_as< quxlang::antestatal_struct >().fields)
            {
                add_static_snapshots_from_antestatal_value(result, field.second);
            }
            return;
        }
        if (value.type_is< quxlang::antestatal_fusion >())
        {
            quxlang::antestatal_fusion const& fusion = value.get_as< quxlang::antestatal_fusion >();
            if (fusion.state.type_is< quxlang::antestatal_fusion_active >())
            {
                quxlang::antestatal_fusion_active const& active = fusion.state.get_as< quxlang::antestatal_fusion_active >();
                if (active.payload.has_value())
                {
                    add_static_snapshots_from_antestatal_value(result, active.payload.value());
                }
            }
        }
    }
} // namespace

auto quxlang::vmir2::reachable_blocks(functanoid_routine3 const& routine, dependency_set set) -> std::set< block_index >
{
    std::set< block_index > result;
    if (routine.blocks.empty())
    {
        return result;
    }

    std::vector< block_index > pending{block_index(0)};
    result.insert(block_index(0));
    while (!pending.empty())
    {
        block_index const index = pending.back();
        pending.pop_back();
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        if (!block.terminator.has_value())
        {
            continue;
        }

        auto enqueue = [&](block_index target)
        {
            if (result.insert(target).second)
            {
                pending.push_back(target);
            }
        };

        vm_terminator const& terminator = *block.terminator;
        if (terminator.type_is< jump >())
        {
            enqueue(terminator.as< jump >().target);
        }
        else if (terminator.type_is< branch >())
        {
            enqueue(terminator.as< branch >().target_true);
            enqueue(terminator.as< branch >().target_false);
        }
        else if (terminator.type_is< tablebranch >())
        {
            tablebranch const& table = terminator.as< tablebranch >();
            for (block_index const target : table.targets)
            {
                enqueue(target);
            }
            enqueue(table.default_target);
        }
        else if (terminator.type_is< runtime_constexpr >())
        {
            runtime_constexpr const& runtime_branch = terminator.as< runtime_constexpr >();
            enqueue(set == dependency_set::native ? runtime_branch.target_native : runtime_branch.target_constexpr);
        }
        else if (terminator.type_is< initguard_try_acquire >())
        {
            enqueue(terminator.as< initguard_try_acquire >().target_acquired);
            enqueue(terminator.as< initguard_try_acquire >().target_already_initialized);
        }
    }
    return result;
}

auto quxlang::vmir2::reachable_local_slots(functanoid_routine3 const& routine, dependency_set set) -> std::set< local_index >
{
    std::set< local_index > result;
    for (routine_parameter const& parameter : routine.parameters.positional)
    {
        result.insert(parameter.local_index);
    }
    for (std::pair< std::string const, routine_parameter > const& parameter : routine.parameters.named)
    {
        result.insert(parameter.second.local_index);
    }

    auto record_state = [&](state_map const& state)
    {
        for (std::pair< local_index const, slot_state > const& entry : state)
        {
            result.insert(entry.first);
        }
    };

    for (block_index const index : reachable_blocks(routine, set))
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        state_map state = block.entry_state;
        codegen_state_engine state_engine(state, routine.local_types, routine.parameters);
        if (index == block_index(0) && state.empty())
        {
            state_engine.apply_entry();
        }
        record_state(state);
        for (vm_instruction const& instruction : block.instructions)
        {
            state_engine.apply(instruction);
            record_state(state);
        }

        if (!block.terminator.has_value())
        {
            continue;
        }
        vm_terminator const& terminator = *block.terminator;
        if (terminator.type_is< branch >())
        {
            result.insert(terminator.as< branch >().condition);
        }
        else if (terminator.type_is< tablebranch >())
        {
            result.insert(terminator.as< tablebranch >().index);
        }
        else if (terminator.type_is< initguard_try_acquire >())
        {
            result.insert(terminator.as< initguard_try_acquire >().target_lock);
        }
    }
    return result;
}

auto quxlang::vmir2::directly_required_static_snapshots(functanoid_routine3 const& routine, dependency_set set) -> std::set< static_snapshot_ref >
{
    std::set< static_snapshot_ref > result;
    std::vector< static_snapshot_ref > pending;
    auto enqueue = [&](static_snapshot_ref const& snapshot)
    {
        if (!routine.static_snapshots.contains(snapshot))
        {
            throw compiler_bug("VMIR2 routine references a static snapshot that it does not carry: " + to_string(type_symbol(snapshot)));
        }
        if (result.insert(snapshot).second)
        {
            pending.push_back(snapshot);
        }
    };

    for (block_index const index : reachable_blocks(routine, set))
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        for (vm_instruction const& instruction : block.instructions)
        {
            if (!instruction.type_is< get_antestatal_ref >())
            {
                continue;
            }
            type_symbol const& symbol = instruction.as< get_antestatal_ref >().symbol;
            if (symbol.type_is< static_snapshot_ref >())
            {
                enqueue(symbol.as< static_snapshot_ref >());
            }
        }
    }

    while (!pending.empty())
    {
        static_snapshot_ref snapshot = std::move(pending.back());
        pending.pop_back();
        std::set< static_snapshot_ref > referenced;
        add_static_snapshots_from_antestatal_value(referenced, routine.static_snapshots.at(snapshot).value);
        for (static_snapshot_ref const& dependency : referenced)
        {
            enqueue(dependency);
        }
    }
    return result;
}

void quxlang::vmir2::validate_dependency_path(functanoid_routine3 const& routine, dependency_set set)
{
    for (block_index const index : reachable_blocks(routine, set))
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        for (vm_instruction const& instruction : block.instructions)
        {
            if (instruction.type_is< lowering_error >())
            {
                throw lowering_compilation_error(instruction.as< lowering_error >().message);
            }
        }
    }
}

auto quxlang::vmir2::directly_instantiated_functanoids(functanoid_routine3 const& routine, dependency_set set) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    std::set< block_index > const reachable = reachable_blocks(routine, set);

    auto is_cleanup_alias = [](slot_state const& slot) -> bool
    {
        return slot.delegate_of.has_value() || slot.array_delegate_of_initializer.has_value() || slot.destroy_delegate || slot.is_projection;
    };
    auto add_slot_destructor = [&](local_index slot, slot_state const& state)
    {
        if (state.nontrivial_dtor.has_value())
        {
            add_functanoid(result, state.nontrivial_dtor->func);
        }
        type_symbol const& slot_type = routine.local_types.at(static_cast< std::uint64_t >(slot)).type;
        std::map< type_symbol, type_symbol >::const_iterator const dtor = routine.non_trivial_dtors.find(slot_type);
        if (dtor != routine.non_trivial_dtors.end())
        {
            add_functanoid(result, dtor->second);
        }
    };
    auto add_edge_destructors = [&](state_map const& current, state_map const& target, std::set< local_index > const& excluded)
    {
        for (std::pair< local_index const, slot_state > const& slot_entry : current)
        {
            bool const survives = target.contains(slot_entry.first) && target.at(slot_entry.first).alive();
            if (!survives && slot_entry.second.dtor_enabled() && !is_cleanup_alias(slot_entry.second) && !excluded.contains(slot_entry.first))
            {
                add_slot_destructor(slot_entry.first, slot_entry.second);
            }
        }
    };

    std::set< local_index > destroy_parameter_slots;
    for (routine_parameter const& parameter : routine.parameters.positional)
    {
        if (parameter.type.type_is< dvalue_slot >()) destroy_parameter_slots.insert(parameter.local_index);
    }
    for (std::pair< std::string const, routine_parameter > const& parameter : routine.parameters.named)
    {
        if (parameter.second.type.type_is< dvalue_slot >()) destroy_parameter_slots.insert(parameter.second.local_index);
    }

    for (block_index const index : reachable)
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        state_map exit_state = block.entry_state;
        codegen_state_engine state_engine(exit_state, routine.local_types, routine.parameters);
        for (vm_instruction const& instruction : block.instructions)
        {
            if (instruction.type_is< invoke >())
            {
                add_functanoid(result, instruction.as< invoke >().what);
            }
            else if (instruction.type_is< get_procedure_ptr >())
            {
                add_functanoid(result, instruction.as< get_procedure_ptr >().routine);
            }
            else if (instruction.type_is< interface_init >())
            {
                for (auto const& [_, routine_symbol] : instruction.as< interface_init >().functions)
                {
                    add_functanoid(result, routine_symbol);
                }
            }
            else if (instruction.type_is< interface_invoke >())
            {
                interface_invoke const& invoke = instruction.as< interface_invoke >();
                if (invoke.default_function.has_value())
                {
                    add_functanoid(result, *invoke.default_function);
                }
            }
            else if (instruction.type_is< destroy >())
            {
                local_index const slot = instruction.as< destroy >().of;
                add_slot_destructor(slot, exit_state.at(slot));
            }

            state_engine.apply(instruction);
            if (instruction.type_is< defer_nontrivial_dtor >())
            {
                defer_nontrivial_dtor const& deferred = instruction.as< defer_nontrivial_dtor >();
                exit_state.at(deferred.on_value).nontrivial_dtor = dtor_spec{
                    .func = deferred.func,
                    .args = deferred.args,
                };
            }
        }

        if (!block.terminator.has_value() || block.terminator->type_is< panic >())
        {
            continue;
        }

        vm_terminator const& terminator = *block.terminator;
        if (terminator.type_is< ret >())
        {
            state_map normal_exit;
            codegen_state_engine(normal_exit, routine.local_types, routine.parameters).apply_normal_exit();
            add_edge_destructors(exit_state, normal_exit, destroy_parameter_slots);
            continue;
        }

        std::vector< block_index > targets;
        if (terminator.type_is< jump >())
        {
            targets.push_back(terminator.as< jump >().target);
        }
        else if (terminator.type_is< branch >())
        {
            targets.push_back(terminator.as< branch >().target_true);
            targets.push_back(terminator.as< branch >().target_false);
        }
        else if (terminator.type_is< tablebranch >())
        {
            targets = terminator.as< tablebranch >().targets;
            targets.push_back(terminator.as< tablebranch >().default_target);
        }
        else if (terminator.type_is< runtime_constexpr >())
        {
            runtime_constexpr const& runtime_branch = terminator.as< runtime_constexpr >();
            targets.push_back(set == dependency_set::native ? runtime_branch.target_native : runtime_branch.target_constexpr);
        }
        else if (terminator.type_is< initguard_try_acquire >())
        {
            targets.push_back(terminator.as< initguard_try_acquire >().target_acquired);
            targets.push_back(terminator.as< initguard_try_acquire >().target_already_initialized);
        }
        for (block_index const target : targets)
        {
            add_edge_destructors(exit_state, routine.blocks.at(static_cast< std::uint64_t >(target)).entry_state, {});
        }
    }

    for (static_snapshot_ref const& snapshot : directly_required_static_snapshots(routine, set))
    {
        localdata_entry const& localdata = routine.static_snapshots.at(snapshot);
        add_functanoids_from_antestatal_value(result, localdata.value, localdata.type);
    }

    return result;
}

auto quxlang::vmir2::directly_instantiated_functanoids(antestatal_value const& value, std::optional< type_symbol > type) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    add_functanoids_from_antestatal_value(result, value, std::move(type));
    return result;
}

auto quxlang::vmir2::directly_referenced_antestatal_globals(functanoid_routine3 const& routine, dependency_set set) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    std::set< type_symbol > localdata_roots;
    for (static_snapshot_ref const& snapshot : directly_required_static_snapshots(routine, set))
    {
        localdata_entry const& localdata = routine.static_snapshots.at(snapshot);
        localdata_roots.insert(type_symbol(snapshot));
        add_antestatal_globals_from_antestatal_value(result, localdata.value, localdata.type);
    }

    for (block_index const index : reachable_blocks(routine, set))
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        for (vm_instruction const& instruction : block.instructions)
        {
            if (!instruction.type_is< get_antestatal_ref >())
            {
                continue;
            }

            type_symbol const& symbol = instruction.as< get_antestatal_ref >().symbol;
            if (localdata_roots.contains(symbol))
            {
                continue;
            }

            add_antestatal_global(result, symbol);
        }
    }

    return result;
}

auto quxlang::vmir2::directly_referenced_global_roots(functanoid_routine3 const& routine, dependency_set set) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    for (block_index const index : reachable_blocks(routine, set))
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        for (vm_instruction const& instruction : block.instructions)
        {
            if (instruction.type_is< get_object_ref >())
            {
                result.insert(instruction.as< get_object_ref >().symbol);
            }
        }
    }

    return result;
}

auto quxlang::vmir2::directly_referenced_antestatal_globals(antestatal_value const& value, std::optional< type_symbol > type) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    add_antestatal_globals_from_antestatal_value(result, value, std::move(type));
    return result;
}

auto quxlang::vmir2::directly_required_type_placements(functanoid_routine3 const& routine, dependency_set set) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    add_routine_surface_types(result, routine, reachable_local_slots(routine, set), directly_required_static_snapshots(routine, set));
    return result;
}

auto quxlang::vmir2::directly_required_struct_layouts(functanoid_routine3 const& routine, dependency_set set) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    for (type_symbol const& type : directly_required_type_placements(routine, set))
    {
        if (type_might_have_layout(type))
        {
            result.insert(type);
        }
    }
    return result;
}

auto quxlang::vmir2::directly_required_fusion_layouts(functanoid_routine3 const& routine, dependency_set set) -> std::set< type_symbol >
{
    std::set< type_symbol > result;
    auto add_slot_type = [&](local_index slot)
    {
        type_symbol type = routine.local_types.at(static_cast< std::uint64_t >(slot)).type;
        if (is_ref(type))
        {
            type = remove_ref(type);
        }
        result.insert(std::move(type));
    };

    for (block_index const index : reachable_blocks(routine, set))
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        for (vm_instruction const& instruction : block.instructions)
        {
            rpnx::apply_visitor< void >(instruction, [&](auto&& concrete)
            {
                using instruction_type = std::decay_t< decltype(concrete) >;
                if constexpr (std::is_same_v< instruction_type, fusion_active_index > ||
                              std::is_same_v< instruction_type, fusion_has_alternative > ||
                              std::is_same_v< instruction_type, fusion_is_valueless > ||
                              std::is_same_v< instruction_type, fusion_storage_ref >)
                {
                    add_slot_type(concrete.subject);
                }
                else if constexpr (std::is_same_v< instruction_type, fusion_set_active > ||
                                   std::is_same_v< instruction_type, fusion_set_valueless >)
                {
                    add_slot_type(concrete.target);
                }
                else if constexpr (std::is_same_v< instruction_type, fusion_swap_boxed_state >)
                {
                    add_slot_type(concrete.a);
                    add_slot_type(concrete.b);
                }
            });
        }
    }
    return result;
}
