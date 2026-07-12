// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/vmir_dependencies_spec.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

#include <type_traits>

namespace
{
    void record_functanoid(quxlang::dependencies& result, quxlang::type_symbol const& symbol, std::optional< quxlang::source_location > location)
    {
        std::pair< std::map< quxlang::type_symbol, std::optional< quxlang::source_location > >::iterator, bool > const insertion = result.functanoids.emplace(symbol, location);
        if (!insertion.second && !insertion.first->second.has_value() && location.has_value())
        {
            insertion.first->second = location;
        }
    }
}

namespace
{
    auto scan_routine(quxlang::vmir2::functanoid_routine3 const& routine, quxlang::dependency_set set) -> quxlang::dependencies
    {
        quxlang::vmir2::validate_dependency_path(routine, set);
        quxlang::dependencies result;
        result.antestatal_globals = quxlang::vmir2::directly_referenced_antestatal_globals(routine, set);
        result.global_roots = quxlang::vmir2::directly_referenced_global_roots(routine, set);
        result.type_placements = quxlang::vmir2::directly_required_type_placements(routine, set);
        result.struct_layouts = quxlang::vmir2::directly_required_struct_layouts(routine, set);

        for (quxlang::vmir2::block_index const index : quxlang::vmir2::reachable_blocks(routine, set))
        {
            quxlang::vmir2::executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
            for (auto const& [_, slot] : block.entry_state)
            {
                if (slot.nontrivial_dtor.has_value())
                {
                    record_functanoid(result, slot.nontrivial_dtor->func, std::nullopt);
                }
            }
            for (quxlang::vmir2::vm_instruction const& instruction : block.instructions)
            {
                std::optional< quxlang::source_location > const location = quxlang::vmir2::get_location(instruction);
                rpnx::apply_visitor< void >(instruction, [&](auto const& concrete)
                {
                    using instruction_type = std::decay_t< decltype(concrete) >;
                    if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::invoke >) record_functanoid(result, concrete.what, location);
                    else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::defer_nontrivial_dtor >) record_functanoid(result, concrete.func, location);
                    else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::get_procedure_ptr >) record_functanoid(result, concrete.routine, location);
                    else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::interface_init >)
                        for (auto const& [_, function] : concrete.functions) record_functanoid(result, function, location);
                    else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::interface_invoke >)
                    {
                        if (concrete.default_function.has_value()) record_functanoid(result, *concrete.default_function, location);
                    }
                    else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::assert_instr >) result.runtime_dependencies.insert(quxlang::vmir_runtime_dependency::assert_fail);
                    else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::initguard_complete >) result.runtime_dependencies.insert(quxlang::vmir_runtime_dependency::initguard_complete);
                    else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::initguard_abort >) result.runtime_dependencies.insert(quxlang::vmir_runtime_dependency::initguard_abort);
                });
            }
            if (block.terminator.has_value() && block.terminator->type_is< quxlang::vmir2::initguard_try_acquire >())
            {
                result.runtime_dependencies.insert(quxlang::vmir_runtime_dependency::initguard_try_acquire);
            }
        }
        for (auto const& [_, dtor] : routine.non_trivial_dtors) record_functanoid(result, dtor, std::nullopt);
        return result;
    }
}

rpnx::querygraph::coroutine< quxlang::direct_dependencies_spec > quxlang::direct_dependencies_impl(direct_dependencies_input input)
{
    if (input.symbol.type_is< instanciation_reference >())
    {
        vmir2::functanoid_routine3 const& routine = co_await rpnx::querygraph::request< vm_procedure3_query >(input.symbol.as< instanciation_reference >());
        co_return scan_routine(routine, input.set);
    }

    ast2_symboid const& symboid = co_await rpnx::querygraph::request< symboid_query >(input.symbol);
    if (symboid.type_is< ast2_test >())
    {
        ast2_test_mode const mode = symboid.as< ast2_test >().mode;
        if (input.set == dependency_set::native && (mode == ast2_test_mode::unit_only || mode == ast2_test_mode::dual))
        {
            vmir2::functanoid_routine3 const& routine = co_await rpnx::querygraph::request< unit_test_vmir_query >(input.symbol);
            co_return scan_routine(routine, input.set);
        }
        if (input.set == dependency_set::constexpr_ && (mode == ast2_test_mode::static_only || mode == ast2_test_mode::dual))
        {
            vmir2::functanoid_routine3 const& routine = co_await rpnx::querygraph::request< static_test_vmir_query >(input.symbol);
            co_return scan_routine(routine, input.set);
        }
        throw compiler_bug("Requested dependency set is unavailable for test symbol: " + to_string(input.symbol));
    }

    type_symbol const type = co_await rpnx::querygraph::request< variable_type_query >(input.symbol);
    antestatal_value const& value = co_await rpnx::querygraph::request< antestatal_static_value_query >(input.symbol);
    dependencies value_dependencies;
    for (type_symbol const& functanoid : vmir2::directly_instantiated_functanoids(value, type)) record_functanoid(value_dependencies, functanoid, std::nullopt);
    value_dependencies.antestatal_globals = vmir2::directly_referenced_antestatal_globals(value, type);
    co_return value_dependencies;
}
