// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/vmir_dependencies_spec.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>

#include "vmir_dependency_scanning.hpp"

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

namespace quxlang::detail
{
    auto scan_routine_dependencies(vmir2::functanoid_routine3 const& routine, dependency_set set) -> dependencies
    {
        vmir2::validate_dependency_path(routine, set);
        dependencies result;
        result.antestatal_globals = vmir2::directly_referenced_antestatal_globals(routine, set);
        result.global_roots = vmir2::directly_referenced_global_roots(routine, set);
        result.type_placements = vmir2::directly_required_type_placements(routine, set);
        result.struct_layouts = vmir2::directly_required_struct_layouts(routine, set);
        result.fusion_layouts = vmir2::directly_required_fusion_layouts(routine, set);
        result.static_snapshots = vmir2::directly_required_static_snapshots(routine, set);

        for (vmir2::block_index const index : vmir2::reachable_blocks(routine, set))
        {
            vmir2::executable_block const& block = routine.blocks.at(static_cast< std::uint64_t >(index));
            for (vmir2::vm_instruction const& instruction : block.instructions)
            {
                std::optional< source_location > const location = vmir2::get_location(instruction);
                rpnx::apply_visitor< void >(instruction, [&](auto const& concrete)
                {
                    using instruction_type = std::decay_t< decltype(concrete) >;
                    if constexpr (std::is_same_v< instruction_type, vmir2::invoke >) record_functanoid(result, concrete.what, location);
                    else if constexpr (std::is_same_v< instruction_type, vmir2::get_procedure_ptr >) record_functanoid(result, concrete.routine, location);
                    else if constexpr (std::is_same_v< instruction_type, vmir2::interface_init >)
                        for (auto const& [_, function] : concrete.functions) record_functanoid(result, function, location);
                    else if constexpr (std::is_same_v< instruction_type, vmir2::interface_invoke >)
                    {
                        if (concrete.default_function.has_value()) record_functanoid(result, *concrete.default_function, location);
                    }
                    else if constexpr (std::is_same_v< instruction_type, vmir2::assert_instr >) result.runtime_dependencies.insert(vmir_runtime_dependency::assert_fail);
                    else if constexpr (std::is_same_v< instruction_type, vmir2::initguard_complete >) result.runtime_dependencies.insert(vmir_runtime_dependency::initguard_complete);
                    else if constexpr (std::is_same_v< instruction_type, vmir2::initguard_abort >) result.runtime_dependencies.insert(vmir_runtime_dependency::initguard_abort);
                });
            }
            if (block.terminator.has_value() && block.terminator->type_is< vmir2::initguard_try_acquire >())
            {
                result.runtime_dependencies.insert(vmir_runtime_dependency::initguard_try_acquire);
            }
            if (block.terminator.has_value() && block.terminator->type_is< vmir2::panic >())
            {
                result.runtime_dependencies.insert(vmir_runtime_dependency::panic);
            }
        }
        for (type_symbol const& functanoid : vmir2::directly_instantiated_functanoids(routine, set))
        {
            record_functanoid(result, functanoid, std::nullopt);
        }
        return result;
    }

    auto scan_constexpr_static_dependencies(antestatal_value const& value, type_symbol const& type) -> dependencies
    {
        dependencies result;
        for (type_symbol const& functanoid : vmir2::directly_instantiated_functanoids(value, type))
        {
            record_functanoid(result, functanoid, std::nullopt);
        }
        result.antestatal_globals = vmir2::directly_referenced_antestatal_globals(value, type);
        return result;
    }
} // namespace quxlang::detail

rpnx::querygraph::coroutine< quxlang::direct_dependencies_spec > quxlang::direct_dependencies_impl(direct_dependencies_input input)
{
    symbol_kind const kind = co_await rpnx::querygraph::request< symbol_type_query >(input.symbol);
    ast2_symboid const& symboid = co_await rpnx::querygraph::request< symboid_query >(input.symbol);

    if (kind == symbol_kind::test)
    {
        if (!symboid.type_is< ast2_test >())
        {
            throw compiler_bug("Test dependency source has no test declaration: " + to_string(input.symbol));
        }
        ast2_test_mode const mode = symboid.as< ast2_test >().mode;
        if (input.set == dependency_set::native && (mode == ast2_test_mode::unit_only || mode == ast2_test_mode::dual))
        {
            vmir2::functanoid_routine3 const& routine = co_await rpnx::querygraph::request< unit_test_vmir_query >(input.symbol);
            co_return detail::scan_routine_dependencies(routine, input.set);
        }
        if (input.set == dependency_set::constexpr_ && (mode == ast2_test_mode::static_only || mode == ast2_test_mode::dual))
        {
            vmir2::functanoid_routine3 const& routine = co_await rpnx::querygraph::request< static_test_vmir_query >(input.symbol);
            co_return detail::scan_routine_dependencies(routine, input.set);
        }
        throw compiler_bug("Requested dependency set is unavailable for test symbol: " + to_string(input.symbol));
    }

    if (symboid.type_is< ast2_asm_procedure_declaration >())
    {
        dependencies asm_dependencies;
        ast2_asm_procedure_declaration const& declaration = symboid.as< ast2_asm_procedure_declaration >();
        for (ast2_asm_instruction const& instruction : declaration.instructions)
        {
            for (ast2_asm_operand const& operand : instruction.operands)
            {
                for (ast2_asm_operand_component const& component : operand.components)
                {
                    if (component.type_is< ast2_procedure_ref >())
                    {
                        ast2_procedure_ref const& procedure_ref = component.get_as< ast2_procedure_ref >();
                        std::optional< type_symbol > resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                            .context = input.symbol,
                            .type = procedure_ref.functanoid,
                        });
                        if (!resolved.has_value())
                        {
                            throw semantic_compilation_error("PROCEDURE_REF target could not be resolved: " + to_string(procedure_ref.functanoid));
                        }

                        if (resolved->type_is< temploid_reference >())
                        {
                            temploid_reference const& selection = resolved->as< temploid_reference >();
                            std::optional< temploid_ensig > const formal_ensig =
                                co_await rpnx::querygraph::request< temploid_formal_ensig_query >(selection);
                            if (!formal_ensig.has_value())
                            {
                                throw semantic_compilation_error("Cannot resolve selected PROCEDURE_REF overload: " + to_string(procedure_ref.functanoid));
                            }
                            if (overload_has_unspecialized_parameters(*formal_ensig))
                            {
                                throw semantic_compilation_error("Cannot emit uninstantiated PROCEDURE_REF target: " + to_string(procedure_ref.functanoid));
                            }
                            *resolved = instanciation_reference{
                                .temploid = selection,
                                .params = instantiate_declared_overload(*formal_ensig),
                            };
                        }
                        else if (!resolved->type_is< instanciation_reference >())
                        {
                            initialization_reference initialization;
                            if (resolved->type_is< initialization_reference >())
                            {
                                initialization = resolved->as< initialization_reference >();
                            }
                            else
                            {
                                initialization.initializee = *resolved;
                            }
                            std::optional< instanciation_reference > const instanciation =
                                co_await rpnx::querygraph::request< instanciation_query >(initialization);
                            if (!instanciation.has_value())
                            {
                                throw semantic_compilation_error("PROCEDURE_REF target is not callable as a concrete function: " + to_string(procedure_ref.functanoid));
                            }
                            *resolved = *instanciation;
                        }
                        record_functanoid(asm_dependencies, *resolved, std::nullopt);
                    }
                    else if (component.type_is< ast2_object_ref >())
                    {
                        ast2_object_ref const& object_ref = component.get_as< ast2_object_ref >();
                        std::optional< type_symbol > const resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                            .context = input.symbol,
                            .type = object_ref.object,
                        });
                        if (!resolved.has_value())
                        {
                            throw semantic_compilation_error("OBJECT_REF target could not be resolved: " + to_string(object_ref.object));
                        }
                        asm_dependencies.global_roots.insert(*resolved);
                    }
                }
            }
        }
        co_return asm_dependencies;
    }

    if (symboid.type_is< ast2_extern_procedure >())
    {
        co_return dependencies{};
    }

    bool const instantiated_nonroutine_entity =
        symboid.type_is< ast2_struct_declaration >() ||
        symboid.type_is< ast2_union_declaration >() ||
        symboid.type_is< ast2_variant_declaration >() ||
        symboid.type_is< ast2_enum_declaration >() ||
        symboid.type_is< ast2_flagset_declaration >() ||
        symboid.type_is< ast2_interface_declaration >() ||
        symboid.type_is< ast2_implementation_declaration >() ||
        symboid.type_is< ast2_variable_declaration >();
    if (kind == symbol_kind::funtanoid ||
        (input.symbol.type_is< instanciation_reference >() && !instantiated_nonroutine_entity))
    {
        if (!input.symbol.type_is< instanciation_reference >())
        {
            throw compiler_bug("Routine dependency source is not an instanciation reference: " + to_string(input.symbol));
        }
        vmir2::functanoid_routine3 const& routine =
            co_await rpnx::querygraph::request< vm_procedure3_query >(input.symbol.as< instanciation_reference >());
        co_return detail::scan_routine_dependencies(routine, input.set);
    }

    if (kind != symbol_kind::global_variable)
    {
        co_return dependencies{};
    }
    if (!(co_await rpnx::querygraph::request< global_is_antestatal_static_query >(input.symbol)))
    {
        co_return dependencies{};
    }

    type_symbol const type = co_await rpnx::querygraph::request< variable_type_query >(input.symbol);
    antestatal_value const& value = co_await rpnx::querygraph::request< antestatal_static_value_query >(input.symbol);
    dependencies value_dependencies;
    class_kind const type_kind = co_await rpnx::querygraph::request< class_type_query >(type);
    if (type_kind == class_kind::union_ || type_kind == class_kind::variant)
    {
        value_dependencies.fusion_layouts.insert(type);
        value_dependencies.type_placements.insert(type);
    }
    std::optional< type_symbol > fusion_payload_type;
    if (value.type_is< antestatal_fusion >())
    {
        antestatal_fusion const& fusion_value = value.get_as< antestatal_fusion >();
        if (fusion_value.alternative.has_value() && !fusion_value.payload.empty())
        {
            std::uint64_t const alternative = *fusion_value.alternative;
            if (type_kind == class_kind::union_)
            {
                union_info const& info = co_await rpnx::querygraph::request< union_info_query >(type);
                if (alternative >= info.options.size())
                {
                    throw compiler_bug("Antestatal UNION value has an invalid active alternative");
                }
                fusion_payload_type = info.options.at(alternative).type;
            }
            else if (type_kind == class_kind::variant)
            {
                variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(type);
                if (alternative >= info.alternatives.size())
                {
                    throw compiler_bug("Antestatal VARIANT value has an invalid active alternative");
                }
                fusion_payload_type = info.alternatives.at(alternative);
            }
        }
    }

    if (fusion_payload_type.has_value())
    {
        for (antestatal_value const& payload : value.get_as< antestatal_fusion >().payload)
        {
            for (type_symbol const& functanoid : vmir2::directly_instantiated_functanoids(payload, *fusion_payload_type))
            {
                record_functanoid(value_dependencies, functanoid, std::nullopt);
            }
            std::set< type_symbol > const payload_globals = vmir2::directly_referenced_antestatal_globals(payload, *fusion_payload_type);
            value_dependencies.antestatal_globals.insert(payload_globals.begin(), payload_globals.end());
        }
    }
    else
    {
        for (type_symbol const& functanoid : vmir2::directly_instantiated_functanoids(value, type))
        {
            record_functanoid(value_dependencies, functanoid, std::nullopt);
        }
        value_dependencies.antestatal_globals = vmir2::directly_referenced_antestatal_globals(value, type);
    }
    co_return value_dependencies;
}
