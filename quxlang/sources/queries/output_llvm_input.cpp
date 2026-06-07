// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/manipulators/mangler.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/queries/specs/output_llvm_input_spec.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>
#include <quxlang/vmir2/source_index.hpp>

#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    struct llvm_packet_support_data
    {
        std::map< quxlang::type_symbol, std::string > procedure_linksymbols;
        std::map< quxlang::type_symbol, quxlang::type_symbol > object_reference_types;
        std::map< quxlang::type_symbol, quxlang::antestatal_value > antestatal_constants;
        std::map< quxlang::type_symbol, quxlang::initialization_type > global_init_types;
        std::map< quxlang::type_symbol, std::vector< quxlang::interface_slot_key > > interface_slots;
        std::map< quxlang::type_symbol, quxlang::enum_info > enum_infos;
        std::map< quxlang::type_symbol, quxlang::flagset_info > flagset_infos;
        std::map< quxlang::type_symbol, quxlang::class_layout > class_layouts;
        std::map< quxlang::type_symbol, quxlang::type_placement_info > type_placements;
    };

    struct collected_routine_tree
    {
        std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 > routines;
        std::map< quxlang::type_symbol, quxlang::asm_procedure > asm_routines;
        std::set< quxlang::type_symbol > object_references;
        llvm_packet_support_data support;
    };

    auto parse_type_symbol_text(std::string const& text) -> quxlang::type_symbol
    {
        auto ctx = quxlang::parsers::make_unlocated_parsing_context(text);
        auto result = quxlang::parsers::parse_type_symbol(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw quxlang::syntax_compilation_error("Input not fully parsed");
        }
        return result;
    }

    auto is_main_function_object_symbol(quxlang::type_symbol const& symbol) -> bool
    {
        return symbol.type_is< quxlang::builtin_symbol >() && symbol.get_as< quxlang::builtin_symbol >().name == "MAIN_FUNCTION";
    }

    auto main_function_object_type() -> quxlang::type_symbol
    {
        quxlang::procedure_type procedure;
        procedure.signature.return_type = quxlang::int_type{.bits = 32, .has_sign = true};
        return procedure;
    }

    auto make_dependency_traceback(
        std::vector< quxlang::trace_frame > const& caller_traceback,
        quxlang::type_symbol const& caller,
        std::optional< quxlang::source_location > location) -> std::vector< quxlang::trace_frame >
    {
        std::vector< quxlang::trace_frame > result;
        result.push_back(quxlang::trace_frame{
            .trace_context = "referenced from " + quxlang::to_string(caller),
            .location = location,
        });
        result.insert(result.end(), caller_traceback.begin(), caller_traceback.end());
        return result;
    }

    void record_referenced_functanoid(
        std::map< quxlang::type_symbol, std::optional< quxlang::source_location > >& result,
        quxlang::type_symbol const& symbol,
        std::optional< quxlang::source_location > location)
    {
        std::pair< std::map< quxlang::type_symbol, std::optional< quxlang::source_location > >::iterator, bool > insertion = result.emplace(symbol, location);
        if (!insertion.second && !insertion.first->second.has_value() && location.has_value())
        {
            insertion.first->second = location;
        }
    }

    auto directly_referenced_functanoid_locations(quxlang::vmir2::functanoid_routine3 const& routine)
        -> std::map< quxlang::type_symbol, std::optional< quxlang::source_location > >
    {
        std::map< quxlang::type_symbol, std::optional< quxlang::source_location > > result;

        for (auto const& dtor_entry : routine.non_trivial_dtors)
        {
            record_referenced_functanoid(result, dtor_entry.second, std::nullopt);
        }

        for (quxlang::vmir2::executable_block const& block : routine.blocks)
        {
            for (auto const& slot_entry : block.entry_state)
            {
                if (slot_entry.second.nontrivial_dtor.has_value())
                {
                    record_referenced_functanoid(result, slot_entry.second.nontrivial_dtor->func, std::nullopt);
                }
            }

            for (quxlang::vmir2::vm_instruction const& instruction : block.instructions)
            {
                std::optional< quxlang::source_location > const location = quxlang::vmir2::get_location(instruction);
                rpnx::apply_visitor< void >(
                    instruction,
                    [&](auto const& concrete_instruction) -> void
                    {
                        if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, quxlang::vmir2::invoke >)
                        {
                            record_referenced_functanoid(result, concrete_instruction.what, location);
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, quxlang::vmir2::defer_nontrivial_dtor >)
                        {
                            record_referenced_functanoid(result, concrete_instruction.func, location);
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, quxlang::vmir2::get_procedure_ptr >)
                        {
                            record_referenced_functanoid(result, concrete_instruction.routine, location);
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, quxlang::vmir2::interface_init >)
                        {
                            for (auto const& function_entry : concrete_instruction.functions)
                            {
                                record_referenced_functanoid(result, function_entry.second, location);
                            }
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, quxlang::vmir2::interface_invoke >)
                        {
                            if (concrete_instruction.default_function.has_value())
                            {
                                record_referenced_functanoid(result, *concrete_instruction.default_function, location);
                            }
                        }
                    });
            }
        }

        return result;
    }

    auto directly_referenced_functanoid_locations(quxlang::ast2_asm_procedure_declaration const& routine)
        -> std::map< quxlang::type_symbol, std::optional< quxlang::source_location > >
    {
        std::map< quxlang::type_symbol, std::optional< quxlang::source_location > > result;

        for (quxlang::ast2_asm_instruction const& instruction : routine.instructions)
        {
            for (quxlang::ast2_asm_operand const& operand : instruction.operands)
            {
                for (quxlang::ast2_asm_operand_component const& component : operand.components)
                {
                    if (component.type_is< quxlang::ast2_procedure_ref >())
                    {
                        quxlang::ast2_procedure_ref const& procedure_ref = component.get_as< quxlang::ast2_procedure_ref >();
                        record_referenced_functanoid(result, procedure_ref.functanoid, std::nullopt);
                    }
                }
            }
        }

        return result;
    }

    auto directly_referenced_objects(quxlang::ast2_asm_procedure_declaration const& routine) -> std::set< quxlang::type_symbol >
    {
        std::set< quxlang::type_symbol > result;

        for (quxlang::ast2_asm_instruction const& instruction : routine.instructions)
        {
            for (quxlang::ast2_asm_operand const& operand : instruction.operands)
            {
                for (quxlang::ast2_asm_operand_component const& component : operand.components)
                {
                    if (component.type_is< quxlang::ast2_object_ref >())
                    {
                        result.insert(component.get_as< quxlang::ast2_object_ref >().object);
                    }
                }
            }
        }

        return result;
    }

    auto llvm_optimization_level(quxlang::backend_llvm_options const& options) -> quxlang::llvm_backend::optimization_level
    {
        if (options.mode == quxlang::backend_llvm_mode::debug)
        {
            return quxlang::llvm_backend::optimization_level::debug;
        }
        return quxlang::llvm_backend::optimization_level::release;
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::output_llvm_input_spec > quxlang::output_llvm_input_impl(std::string input)
{
    output_query_output const output_info = co_await rpnx::querygraph::request< output_binary_information_query >(input);
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    backend_llvm_options const llvm_options = co_await rpnx::querygraph::request< output_llvm_backend_options_query >(input);
    machine_target_info const machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});

    if (!target_config.module_configurations.contains(output_info.module_name))
    {
        throw semantic_compilation_error("Output '" + output_info.output_name + "' references unknown module '" + output_info.module_name + "'");
    }

    type_symbol const module_context = absolute_module_reference{.module_name = output_info.module_name};
    type_symbol parsed_entry = parse_type_symbol_text(output_info.main_functanoid);
    type_symbol contextual_entry = with_context(parsed_entry, module_context);
    std::optional< type_symbol > const resolved_entry = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
        .context = module_context,
        .type = contextual_entry,
    });

    if (!resolved_entry.has_value())
    {
        throw semantic_compilation_error("Could not resolve main functanoid '" + output_info.main_functanoid + "' in module '" + output_info.module_name + "'");
    }

    std::optional< instanciation_reference > entry_instanciation;
    if (resolved_entry->type_is< instanciation_reference >())
    {
        entry_instanciation = resolved_entry->as< instanciation_reference >();
    }
    else
    {
        initialization_reference empty_call;
        if (resolved_entry->type_is< initialization_reference >())
        {
            empty_call = resolved_entry->as< initialization_reference >();
        }
        else
        {
            empty_call.initializee = *resolved_entry;
        }

        entry_instanciation = co_await rpnx::querygraph::request< instanciation_query >(std::move(empty_call));
    }

    if (!entry_instanciation.has_value())
    {
        throw semantic_compilation_error("Main functanoid '" + output_info.main_functanoid + "' in module '" + output_info.module_name + "' is not callable as a concrete function");
    }

    instanciation_reference const entry_functanoid = *entry_instanciation;
    if (output_info.type == output_kind::executable)
    {
        if (!entry_functanoid.params.positional.empty() || !entry_functanoid.params.named.empty())
        {
            throw semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + to_string(entry_functanoid));
        }

        type_symbol const return_type = co_await rpnx::querygraph::request< functanoid_return_type_query >(entry_functanoid);
        if (return_type != type_symbol(int_type{.bits = 32, .has_sign = true}))
        {
            throw semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + to_string(entry_functanoid));
        }
    }

    std::optional< type_symbol > runtime_program_start;
    if (target_config.module_configurations.contains("RUNTIME"))
    {
        type_symbol runtime_start = subsymbol{
            .of = absolute_module_reference{.module_name = "RUNTIME"},
            .name = "PROGRAM_START",
        };
        ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(runtime_start);
        if (!symboid.type_is< std::monostate >())
        {
            if (!symboid.type_is< ast2_asm_procedure_declaration >())
            {
                throw semantic_compilation_error("RUNTIME::PROGRAM_START must be an ASM_PROCEDURE");
            }
            runtime_program_start = runtime_start;
        }
    }

    vmir2::functanoid_routine3 const entry_routine = co_await rpnx::querygraph::request< vm_procedure3_query >(entry_functanoid);

    collected_routine_tree tree;
    std::vector< std::pair< instanciation_reference, std::vector< trace_frame > > > pending_functanoids;
    std::set< type_symbol > queued_functanoids;
    tree.routines.emplace(type_symbol(entry_functanoid), entry_routine);

    auto enqueue_functanoid = [&](instanciation_reference const& functanoid, std::vector< trace_frame > traceback) -> void
    {
        type_symbol functanoid_symbol = functanoid;
        if (queued_functanoids.insert(functanoid_symbol).second)
        {
            pending_functanoids.push_back(std::make_pair(functanoid, std::move(traceback)));
        }
    };

    auto enqueue_references =
        [&](type_symbol const& caller, std::map< type_symbol, std::optional< source_location > > const& references, std::vector< trace_frame > const& traceback) -> void
    {
        for (std::pair< type_symbol const, std::optional< source_location > > const& referenced_functanoid : references)
        {
            if (!referenced_functanoid.first.type_is< instanciation_reference >())
            {
                throw compiler_bug("VMIR2 dependency scan returned a non-instanciation reference: " + to_string(referenced_functanoid.first));
            }
            enqueue_functanoid(
                referenced_functanoid.first.as< instanciation_reference >(),
                make_dependency_traceback(traceback, caller, referenced_functanoid.second));
        }
    };

    enqueue_references(type_symbol(entry_functanoid), directly_referenced_functanoid_locations(entry_routine), {});

    if (runtime_program_start.has_value())
    {
        ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(*runtime_program_start);
        ast2_asm_procedure_declaration const& declaration = symboid.get_as< ast2_asm_procedure_declaration >();
        tree.asm_routines.emplace(*runtime_program_start, co_await rpnx::querygraph::request< asm_procedure_from_symbol_query >(*runtime_program_start));
        enqueue_references(*runtime_program_start, directly_referenced_functanoid_locations(declaration), {});
        for (type_symbol const& object_reference : directly_referenced_objects(declaration))
        {
            std::optional< type_symbol > const canonical_object = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                .context = *runtime_program_start,
                .type = object_reference,
            });
            if (!canonical_object.has_value())
            {
                throw semantic_compilation_error("OBJECT_REF target could not be resolved: " + to_string(object_reference));
            }
            tree.object_references.insert(*canonical_object);
        }
    }

    while (!pending_functanoids.empty())
    {
        std::pair< instanciation_reference, std::vector< trace_frame > > pending_functanoid = std::move(pending_functanoids.back());
        pending_functanoids.pop_back();

        instanciation_reference functanoid = std::move(pending_functanoid.first);
        std::vector< trace_frame > dependency_traceback = std::move(pending_functanoid.second);
        type_symbol functanoid_symbol = functanoid;
        if (tree.routines.contains(functanoid_symbol) || tree.asm_routines.contains(functanoid_symbol))
        {
            continue;
        }

        type_symbol asm_declaration_symbol = functanoid_symbol;
        if (functanoid_symbol.type_is< instanciation_reference >())
        {
            asm_declaration_symbol = functanoid_symbol.get_as< instanciation_reference >().temploid.templexoid;
        }

        ast2_symboid const symboid = co_await rpnx::querygraph::request< symboid_query >(asm_declaration_symbol);
        if (symboid.type_is< ast2_asm_procedure_declaration >())
        {
            ast2_asm_procedure_declaration const& declaration = symboid.get_as< ast2_asm_procedure_declaration >();
            tree.asm_routines.emplace(functanoid_symbol, co_await rpnx::querygraph::request< asm_procedure_from_symbol_query >(functanoid_symbol));
            enqueue_references(functanoid_symbol, directly_referenced_functanoid_locations(declaration), dependency_traceback);
            for (type_symbol const& object_reference : directly_referenced_objects(declaration))
            {
                std::optional< type_symbol > const canonical_object = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = functanoid_symbol,
                    .type = object_reference,
                });
                if (!canonical_object.has_value())
                {
                    throw semantic_compilation_error("OBJECT_REF target could not be resolved: " + to_string(object_reference));
                }
                tree.object_references.insert(*canonical_object);
            }
            continue;
        }

        vmir2::functanoid_routine3 procedure;
        try
        {
            procedure = co_await rpnx::querygraph::request< vm_procedure3_query >(functanoid);
        }
        catch (compilation_error& error)
        {
            error.traceback.insert(error.traceback.end(), dependency_traceback.begin(), dependency_traceback.end());
            throw;
        }

        tree.routines.emplace(functanoid_symbol, procedure);
        enqueue_references(functanoid_symbol, directly_referenced_functanoid_locations(procedure), dependency_traceback);
    }

    std::set< type_symbol > seen_types;
    std::vector< type_symbol > pending_types;
    std::set< type_symbol > seen_antestatal_globals;
    std::vector< type_symbol > pending_antestatal_globals;

    auto enqueue_type = [&](type_symbol const& type) -> void
    {
        if (seen_types.insert(type).second)
        {
            pending_types.push_back(type);
        }
    };

    auto enqueue_antestatal_global = [&](type_symbol const& symbol) -> void
    {
        if (seen_antestatal_globals.insert(symbol).second)
        {
            pending_antestatal_globals.push_back(symbol);
        }
    };

    for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine_entry : tree.routines)
    {
        for (type_symbol const& placement_root : vmir2::directly_required_type_placements(routine_entry.second))
        {
            enqueue_type(placement_root);
        }
        for (type_symbol const& antestatal_root : vmir2::directly_referenced_antestatal_globals(routine_entry.second))
        {
            enqueue_antestatal_global(antestatal_root);
        }
        for (type_symbol const& global_root : vmir2::directly_referenced_global_roots(routine_entry.second))
        {
            tree.support.global_init_types[global_root] = co_await rpnx::querygraph::request< global_init_type_query >(global_root);
        }
        for (std::pair< static_snapshot_ref const, vmir2::localdata_entry > const& snapshot_entry : routine_entry.second.static_snapshots)
        {
            type_symbol const snapshot_symbol = type_symbol(snapshot_entry.first);
            tree.support.antestatal_constants[snapshot_symbol] = snapshot_entry.second.value;
            enqueue_type(snapshot_entry.second.type);
        }
    }

    for (type_symbol const& object_reference : tree.object_references)
    {
        if (is_main_function_object_symbol(object_reference))
        {
            type_symbol const object_type = main_function_object_type();
            tree.support.object_reference_types.emplace(object_reference, object_type);
            enqueue_type(object_type);
            continue;
        }

        if (co_await rpnx::querygraph::request< symbol_type_query >(object_reference) != symbol_kind::global_variable)
        {
            throw semantic_compilation_error("OBJECT_REF target is not a global object: " + to_string(object_reference));
        }

        type_symbol const object_type = co_await rpnx::querygraph::request< variable_type_query >(object_reference);
        tree.support.object_reference_types.emplace(object_reference, object_type);
        tree.support.global_init_types[object_reference] = co_await rpnx::querygraph::request< global_init_type_query >(object_reference);
        enqueue_type(object_type);
        if (co_await rpnx::querygraph::request< global_is_antestatal_static_query >(object_reference))
        {
            enqueue_antestatal_global(object_reference);
        }
    }

    while (!pending_antestatal_globals.empty())
    {
        type_symbol symbol = std::move(pending_antestatal_globals.back());
        pending_antestatal_globals.pop_back();

        if (!co_await rpnx::querygraph::request< global_is_antestatal_static_query >(symbol))
        {
            continue;
        }

        tree.support.antestatal_constants[symbol] = co_await rpnx::querygraph::request< antestatal_static_value_query >(symbol);
        enqueue_type(co_await rpnx::querygraph::request< variable_type_query >(symbol));
    }

    while (!pending_types.empty())
    {
        type_symbol type = std::move(pending_types.back());
        pending_types.pop_back();

        bool skip_type_placement_query = false;
        rpnx::apply_visitor< void >(
            type,
            [&](auto const& concrete_type) -> void
            {
                if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, nvalue_slot >)
                {
                    enqueue_type(concrete_type.target);
                    skip_type_placement_query = true;
                }
                else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, dvalue_slot >)
                {
                    enqueue_type(concrete_type.target);
                    skip_type_placement_query = true;
                }
                else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, attached_type_reference >)
                {
                    if (!concrete_type.carrying_type.template type_is< void_type >())
                    {
                        enqueue_type(concrete_type.carrying_type);
                    }
                }
                else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, ptrref_type >)
                {
                    enqueue_type(concrete_type.target);
                }
                else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, array_type >)
                {
                    enqueue_type(concrete_type.element_type);
                }
                else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, array_initializer_type >)
                {
                    enqueue_type(concrete_type.element_type);
                    skip_type_placement_query = true;
                }
                else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, procedure_type >)
                {
                    for (type_symbol const& positional : concrete_type.signature.params.positional)
                    {
                        enqueue_type(positional);
                    }
                    for (std::pair< std::string const, type_symbol > const& named : concrete_type.signature.params.named)
                    {
                        enqueue_type(named.second);
                    }
                    if (concrete_type.signature.return_type.has_value())
                    {
                        enqueue_type(*concrete_type.signature.return_type);
                    }
                    skip_type_placement_query = true;
                }
                else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_type) >, storage >)
                {
                    for (type_symbol const& storable_type : concrete_type.storable_types)
                    {
                        enqueue_type(storable_type);
                    }
                }
            });

        if (std::optional< type_symbol > const atomic_value = atomic_type_argument(type); atomic_value.has_value())
        {
            enqueue_type(*atomic_value);
        }

        if (type.type_is< size_type >())
        {
            tree.support.type_placements[type] = type_placement_info{
                .size = machine.pointer_size_bytes(),
                .alignment = machine.pointer_align(),
            };
        }
        else if (!skip_type_placement_query)
        {
            tree.support.type_placements[type] = co_await rpnx::querygraph::request< type_placement_info_query >(type);
        }

        if (!(type.type_is< subsymbol >() || type.type_is< instanciation_reference >() || type.type_is< readonly_constant >()))
        {
            continue;
        }

        symbol_kind const kind = co_await rpnx::querygraph::request< symbol_type_query >(type);
        if (kind == symbol_kind::interface_)
        {
            std::vector< interface_slot > const slots = co_await rpnx::querygraph::request< interface_slot_list_query >(type);
            std::vector< interface_slot_key > slot_keys;
            slot_keys.reserve(slots.size());
            for (interface_slot const& slot : slots)
            {
                slot_keys.push_back(slot.key);
                for (type_symbol const& positional : slot.key.concrete_params.positional)
                {
                    enqueue_type(positional);
                }
                for (std::pair< std::string const, type_symbol > const& named : slot.key.concrete_params.named)
                {
                    enqueue_type(named.second);
                }
                if (slot.key.concrete_return_type.has_value())
                {
                    enqueue_type(*slot.key.concrete_return_type);
                }
            }
            tree.support.interface_slots[type] = std::move(slot_keys);
            continue;
        }
        if (kind == symbol_kind::enum_)
        {
            tree.support.enum_infos[type] = co_await rpnx::querygraph::request< enum_info_query >(type);
            continue;
        }
        if (kind == symbol_kind::flagset_)
        {
            tree.support.flagset_infos[type] = co_await rpnx::querygraph::request< flagset_info_query >(type);
            continue;
        }

        class_layout const layout = co_await rpnx::querygraph::request< class_layout_query >(type);
        tree.support.class_layouts[type] = layout;
        for (class_field_info const& field : layout.fields)
        {
            enqueue_type(field.type);
        }
    }

    for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine_entry : tree.routines)
    {
        ast2_procedure_ref procedure_ref{.cc = "", .functanoid = routine_entry.first};
        tree.support.procedure_linksymbols.emplace(routine_entry.first, co_await rpnx::querygraph::request< procedure_linksymbol_query >(procedure_ref));
    }
    for (std::pair< type_symbol const, asm_procedure > const& routine_entry : tree.asm_routines)
    {
        ast2_procedure_ref procedure_ref{.cc = "", .functanoid = routine_entry.first};
        tree.support.procedure_linksymbols.emplace(routine_entry.first, co_await rpnx::querygraph::request< procedure_linksymbol_query >(procedure_ref));
    }

    source_file_index const file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    source_bundle const bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});

    llvm_backend::llvm_compilable_unit output_module_unit;
    output_module_unit.target_name = entry_functanoid;
    output_module_unit.target_code = entry_routine;
    output_module_unit.machine_target.machine = machine;
    output_module_unit.machine_target.optimization = llvm_optimization_level(llvm_options);
    output_module_unit.whole_module = true;
    output_module_unit.whole_module_output_kind = output_info.type;
    if (runtime_program_start.has_value() && output_info.type == output_kind::executable && machine.os_type == os::linux && machine.binary_type == binary::elf)
    {
        output_module_unit.executable_entry_symbol = mangle(*runtime_program_start);
    }
    output_module_unit.source_index = rpnx::cow< vmir2::source_index >(vmir2::source_index(file_index, bundle));
    output_module_unit.procedure_linksymbols = tree.support.procedure_linksymbols;
    output_module_unit.object_reference_types = tree.support.object_reference_types;
    output_module_unit.antestatal_constants = tree.support.antestatal_constants;
    output_module_unit.global_init_types = tree.support.global_init_types;
    output_module_unit.interface_slots = tree.support.interface_slots;
    output_module_unit.enum_infos = tree.support.enum_infos;
    output_module_unit.flagset_infos = tree.support.flagset_infos;
    output_module_unit.class_layouts = tree.support.class_layouts;
    output_module_unit.type_placements = tree.support.type_placements;
    for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine_entry : tree.routines)
    {
        if (routine_entry.first != type_symbol(entry_functanoid))
        {
            output_module_unit.inlinable_functions.insert(routine_entry);
        }
    }
    output_module_unit.asm_functions = tree.asm_routines;

    co_return output_module_unit;
}
