// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

#include <optional>
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

        return type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >();
    }

    void add_routine_surface_types(std::set< quxlang::type_symbol >& result, quxlang::vmir2::functanoid_routine3 const& routine)
    {
        for (quxlang::vmir2::local_type const& local_type : routine.local_types)
        {
            add_type_and_components(result, local_type.type);
        }
        for (auto const& [_, param] : routine.parameters.named)
        {
            add_type_and_components(result, param.type);
        }
        for (quxlang::vmir2::routine_parameter const& param : routine.parameters.positional)
        {
            add_type_and_components(result, param.type);
        }
        for (auto const& [_, localdata] : routine.static_snapshots)
        {
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

    for (auto const& [_, dtor] : routine.non_trivial_dtors)
    {
        add_functanoid(result, dtor);
    }

    for (block_index const index : reachable)
    {
        executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
        for (auto const& [_, slot] : block.entry_state)
        {
            if (slot.nontrivial_dtor.has_value())
            {
                add_functanoid(result, slot.nontrivial_dtor->func);
            }
        }

        for (vm_instruction const& instruction : block.instructions)
        {
            if (instruction.type_is< invoke >())
            {
                add_functanoid(result, instruction.as< invoke >().what);
            }
            else if (instruction.type_is< defer_nontrivial_dtor >())
            {
                add_functanoid(result, instruction.as< defer_nontrivial_dtor >().func);
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
        }
    }

    for (auto const& [_, localdata] : routine.static_snapshots)
    {
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
    for (auto const& [symbol, localdata] : routine.static_snapshots)
    {
        localdata_roots.insert(type_symbol(symbol));
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
    add_routine_surface_types(result, routine);
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
