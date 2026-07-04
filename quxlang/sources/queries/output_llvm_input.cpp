// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/manipulators/typeutils.hpp>
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

    auto llvm_optimization_level(quxlang::backend_llvm_options const& options) -> quxlang::llvm_backend::optimization_level
    {
        if (options.mode == quxlang::backend_llvm_mode::debug)
        {
            return quxlang::llvm_backend::optimization_level::debug;
        }
        return quxlang::llvm_backend::optimization_level::release;
    }

    auto runtime_start_name(quxlang::output_kind kind) -> std::string
    {
        if (kind == quxlang::output_kind::unit_test_suite)
        {
            return "UNIT_TESTING_PROGRAM_START";
        }
        return "PROGRAM_START";
    }

    auto unit_test_suite_root_symbol(quxlang::type_symbol const& module_context) -> quxlang::type_symbol
    {
        return quxlang::subsymbol{
            .of = module_context,
            .name = "__UNIT_TEST_SUITE_ROOT",
        };
    }

    auto make_empty_unit_test_suite_root_routine() -> quxlang::vmir2::functanoid_routine3
    {
        quxlang::vmir2::functanoid_routine3 routine;
        routine.blocks.push_back(quxlang::vmir2::executable_block{
            .terminator = quxlang::vmir2::ret{},
            .dbg_name = "unit_test_suite_root",
        });
        routine.block_names.emplace(quxlang::vmir2::block_index(0), "unit_test_suite_root");
        return routine;
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::output_llvm_input_spec > quxlang::output_llvm_input_impl(std::string input)
{
    rpnx::querygraph::request< output_binary_information_query > output_info_request(input);
    rpnx::querygraph::request< target_configuration_query > target_config_request(std::monostate{});
    rpnx::querygraph::request< output_llvm_backend_options_query > llvm_options_request(input);
    rpnx::querygraph::request< machine_info_query > machine_request(std::monostate{});
    rpnx::querygraph::request< source_file_index_query > source_file_index_request(std::monostate{});
    rpnx::querygraph::request< source_bundle_query > source_bundle_request(std::monostate{});

    co_yield rpnx::querygraph::dependency(output_info_request);
    co_yield rpnx::querygraph::dependency(target_config_request);
    co_yield rpnx::querygraph::dependency(llvm_options_request);
    co_yield rpnx::querygraph::dependency(machine_request);
    co_yield rpnx::querygraph::dependency(source_file_index_request);
    co_yield rpnx::querygraph::dependency(source_bundle_request);

    output_query_output const output_info = co_await output_info_request;
    target_configuration const& target_config = co_await target_config_request;
    backend_llvm_options const llvm_options = co_await llvm_options_request;
    machine_target_info const machine = co_await machine_request;
    source_file_index const file_index = co_await source_file_index_request;
    source_bundle const bundle = co_await source_bundle_request;
    vmir2::source_index const source_index(file_index, bundle);

    if (!target_config.module_configurations.contains(output_info.module_name))
    {
        throw semantic_compilation_error("Output '" + output_info.output_name + "' references unknown module '" + output_info.module_name + "'");
    }

    type_symbol const module_context = absolute_module_reference{.module_name = output_info.module_name};
    std::optional< instanciation_reference > entry_functanoid;
    std::optional< type_symbol > entry_functanoid_symbol;
    std::vector< std::pair< type_symbol, rpnx::querygraph::request< unit_test_vmir_query > > > unit_test_requests;

    if (output_info.type == output_kind::unit_test_suite)
    {
        if (output_info.main_functanoid.has_value())
        {
            throw semantic_compilation_error("Output '" + output_info.output_name + "' of type unit_test_suite cannot configure main_functanoid");
        }

        rpnx::querygraph::request< list_unit_tests_query > unit_test_list_request(module_context);
        co_yield rpnx::querygraph::dependency(unit_test_list_request);
        std::set< type_symbol > const unit_tests = co_await unit_test_list_request;
        for (type_symbol const& unit_test : unit_tests)
        {
            unit_test_requests.push_back(std::make_pair(unit_test, rpnx::querygraph::request< unit_test_vmir_query >(unit_test)));
            co_yield rpnx::querygraph::dependency(unit_test_requests.back().second);
        }
    }
    else
    {
        if (!output_info.main_functanoid.has_value())
        {
            throw semantic_compilation_error("Output '" + output_info.output_name + "' requires a main functanoid");
        }

        type_symbol const contextual_entry = with_context(*output_info.main_functanoid, module_context);
        rpnx::querygraph::request< lookup_query > entry_lookup_request(contextual_type_reference{
            .context = module_context,
            .type = contextual_entry,
        });
        co_yield rpnx::querygraph::dependency(entry_lookup_request);

        std::optional< type_symbol > const resolved_entry = co_await entry_lookup_request;
        if (!resolved_entry.has_value())
        {
            throw semantic_compilation_error("Could not resolve main functanoid '" + to_string(*output_info.main_functanoid) + "' in module '" + output_info.module_name + "'");
        }

        if (resolved_entry->type_is< instanciation_reference >())
        {
            entry_functanoid = resolved_entry->as< instanciation_reference >();
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

            rpnx::querygraph::request< instanciation_query > entry_instanciation_request(std::move(empty_call));
            co_yield rpnx::querygraph::dependency(entry_instanciation_request);
            entry_functanoid = co_await entry_instanciation_request;
        }

        if (!entry_functanoid.has_value())
        {
            throw semantic_compilation_error("Main functanoid '" + to_string(*output_info.main_functanoid) + "' in module '" + output_info.module_name + "' is not callable as a concrete function");
        }

        if (output_info.type == output_kind::executable)
        {
            if (!entry_functanoid->params.positional.empty() || !entry_functanoid->params.named.empty())
            {
                throw semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + to_string(*entry_functanoid));
            }

            rpnx::querygraph::request< functanoid_return_type_query > entry_return_type_request(*entry_functanoid);
            co_yield rpnx::querygraph::dependency(entry_return_type_request);
            type_symbol const return_type = co_await entry_return_type_request;
            if (return_type != type_symbol(int_type{.bits = 32, .has_sign = true}))
            {
                throw semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + to_string(*entry_functanoid));
            }
        }

        entry_functanoid_symbol = type_symbol(*entry_functanoid);
    }

    llvm_backend::llvm_compilable_unit output_module_unit;
    output_module_unit.target_name = entry_functanoid_symbol.value_or(unit_test_suite_root_symbol(module_context));
    output_module_unit.machine_target.machine = machine;
    output_module_unit.machine_target.optimization = llvm_optimization_level(llvm_options);
    output_module_unit.whole_module = true;
    output_module_unit.whole_module_output_kind = output_info.type;

    std::optional< rpnx::querygraph::request< vm_procedure3_query > > entry_routine_request;
    if (entry_functanoid.has_value())
    {
        entry_routine_request.emplace(*entry_functanoid);
        co_yield rpnx::querygraph::dependency(*entry_routine_request);
    }

    std::optional< type_symbol > runtime_program_start_candidate;
    std::optional< rpnx::querygraph::request< symboid_query > > runtime_program_start_request;
    std::string const selected_runtime_start_name = runtime_start_name(output_info.type);
    if (target_config.module_configurations.contains("RUNTIME"))
    {
        type_symbol runtime_start = subsymbol{
            .of = absolute_module_reference{.module_name = "RUNTIME"},
            .name = selected_runtime_start_name,
        };
        runtime_program_start_candidate = runtime_start;
        runtime_program_start_request.emplace(runtime_start);
        co_yield rpnx::querygraph::dependency(*runtime_program_start_request);
    }

    if (entry_routine_request.has_value())
    {
        output_module_unit.target_code = co_await *entry_routine_request;
    }
    else
    {
        output_module_unit.target_code = make_empty_unit_test_suite_root_routine();
    }

    for (std::pair< type_symbol, rpnx::querygraph::request< unit_test_vmir_query > >& unit_test_request : unit_test_requests)
    {
        vmir2::functanoid_routine3 unit_test_routine = co_await unit_test_request.second;
        output_module_unit.unit_tests.push_back(llvm_backend::unit_test_entry{
            .name = to_string(unit_test_request.first),
            .procedure_symbol = unit_test_request.first,
        });
        output_module_unit.inlinable_functions.emplace(unit_test_request.first, std::move(unit_test_routine));
    }

    std::optional< type_symbol > runtime_program_start;

    std::vector< std::pair< instanciation_reference, std::vector< trace_frame > > > pending_functanoids;
    std::set< type_symbol > queued_functanoids;
    if (entry_functanoid_symbol.has_value())
    {
        queued_functanoids.insert(*entry_functanoid_symbol);
    }
    std::set< type_symbol > object_references;
    std::set< llvm_backend::runtime_procedure_reference > pending_runtime_procedures;

    auto make_dependency_traceback =
        [](std::vector< trace_frame > const& caller_traceback, type_symbol const& caller, std::optional< source_location > location) -> std::vector< trace_frame >
    {
        std::vector< trace_frame > result;
        result.push_back(trace_frame{
            .trace_context = "referenced from " + to_string(caller),
            .location = location,
        });
        result.insert(result.end(), caller_traceback.begin(), caller_traceback.end());
        return result;
    };

    auto enqueue_functanoid =
        [&](type_symbol const& caller, type_symbol const& referenced_symbol, std::optional< source_location > location, std::vector< trace_frame > const& traceback) -> void
    {
        if (!referenced_symbol.type_is< instanciation_reference >())
        {
            throw compiler_bug("Output dependency scan returned a non-instanciation reference: " + to_string(referenced_symbol));
        }

        instanciation_reference const functanoid = referenced_symbol.as< instanciation_reference >();
        type_symbol const functanoid_symbol = type_symbol(functanoid);
        if (queued_functanoids.insert(functanoid_symbol).second)
        {
            pending_functanoids.push_back(std::make_pair(functanoid, make_dependency_traceback(traceback, caller, location)));
        }
    };

    auto enqueue_vmir_references = [&](type_symbol const& caller, vmir2::functanoid_routine3 const& routine, std::vector< trace_frame > const& traceback) -> void
    {
        for (vmir2::executable_block const& block : routine.blocks)
        {
            for (vmir2::vm_instruction const& instruction : block.instructions)
            {
                std::optional< source_location > const location = vmir2::get_location(instruction);
                rpnx::apply_visitor< void >(
                    instruction,
                    [&](auto const& concrete_instruction) -> void
                    {
                        if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::invoke >)
                        {
                            enqueue_functanoid(caller, concrete_instruction.what, location, traceback);
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::defer_nontrivial_dtor >)
                        {
                            enqueue_functanoid(caller, concrete_instruction.func, location, traceback);
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::get_procedure_ptr >)
                        {
                            enqueue_functanoid(caller, concrete_instruction.routine, location, traceback);
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::interface_init >)
                        {
                            for (std::pair< interface_slot_key const, type_symbol > const& function_entry : concrete_instruction.functions)
                            {
                                enqueue_functanoid(caller, function_entry.second, location, traceback);
                            }
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::interface_invoke >)
                        {
                            if (concrete_instruction.default_function.has_value())
                            {
                                enqueue_functanoid(caller, *concrete_instruction.default_function, location, traceback);
                            }
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::assert_instr >)
                        {
                            (void)concrete_instruction;
                            llvm_backend::runtime_procedure_reference reference{.procedure = llvm_backend::runtime_procedure::assert_fail};
                            if (!output_module_unit.runtime_procedures.contains(reference))
                            {
                                pending_runtime_procedures.insert(std::move(reference));
                            }
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::initguard_complete >)
                        {
                            (void)concrete_instruction;
                            llvm_backend::runtime_procedure_reference reference{.procedure = llvm_backend::runtime_procedure::initguard_complete};
                            if (!output_module_unit.runtime_procedures.contains(reference))
                            {
                                pending_runtime_procedures.insert(std::move(reference));
                            }
                        }
                        else if constexpr (std::is_same_v< std::decay_t< decltype(concrete_instruction) >, vmir2::initguard_abort >)
                        {
                            (void)concrete_instruction;
                            llvm_backend::runtime_procedure_reference reference{.procedure = llvm_backend::runtime_procedure::initguard_abort};
                            if (!output_module_unit.runtime_procedures.contains(reference))
                            {
                                pending_runtime_procedures.insert(std::move(reference));
                            }
                        }
                    });
            }

            if (block.terminator.has_value() && block.terminator->type_is< vmir2::initguard_try_acquire >())
            {
                llvm_backend::runtime_procedure_reference reference{.procedure = llvm_backend::runtime_procedure::initguard_try_acquire};
                if (!output_module_unit.runtime_procedures.contains(reference))
                {
                    pending_runtime_procedures.insert(std::move(reference));
                }
            }

            for (std::pair< vmir2::local_index const, vmir2::slot_state > const& slot_entry : block.entry_state)
            {
                if (slot_entry.second.nontrivial_dtor.has_value())
                {
                    enqueue_functanoid(caller, slot_entry.second.nontrivial_dtor->func, std::nullopt, traceback);
                }
            }
        }

        for (std::pair< type_symbol const, type_symbol > const& dtor_entry : routine.non_trivial_dtors)
        {
            enqueue_functanoid(caller, dtor_entry.second, std::nullopt, traceback);
        }
    };

    auto co_enqueue_asm_references =
        [&](type_symbol const& caller, ast2_asm_procedure_declaration const& routine, std::vector< trace_frame > const& traceback) -> rpnx::querygraph::cosubroutine_impl< output_llvm_input_spec, void >
    {
        for (ast2_asm_instruction const& instruction : routine.instructions)
        {
            for (ast2_asm_operand const& operand : instruction.operands)
            {
                for (ast2_asm_operand_component const& component : operand.components)
                {
                    if (component.type_is< ast2_procedure_ref >())
                    {
                        ast2_procedure_ref const& procedure_ref = component.get_as< ast2_procedure_ref >();
                        rpnx::querygraph::request< lookup_query > lookup_request(contextual_type_reference{
                            .context = caller,
                            .type = procedure_ref.functanoid,
                        });
                        co_yield rpnx::querygraph::dependency(lookup_request);

                        std::optional< type_symbol > const resolved_procedure = co_await lookup_request;
                        if (!resolved_procedure.has_value())
                        {
                            throw semantic_compilation_error("PROCEDURE_REF target could not be resolved: " + to_string(procedure_ref.functanoid));
                        }

                        if (resolved_procedure->type_is< instanciation_reference >())
                        {
                            enqueue_functanoid(caller, *resolved_procedure, std::nullopt, traceback);
                            continue;
                        }

                        if (resolved_procedure->type_is< temploid_reference >())
                        {
                            temploid_reference const& selection = resolved_procedure->get_as< temploid_reference >();
                            rpnx::querygraph::request< temploid_formal_ensig_query > formal_ensig_request(selection);
                            co_yield rpnx::querygraph::dependency(formal_ensig_request);

                            std::optional< temploid_ensig > const formal_ensig = co_await formal_ensig_request;
                            if (!formal_ensig.has_value())
                            {
                                throw semantic_compilation_error("Cannot resolve selected PROCEDURE_REF overload: " + to_string(procedure_ref.functanoid));
                            }
                            if (overload_has_unspecialized_parameters(*formal_ensig))
                            {
                                throw semantic_compilation_error("Cannot emit uninstantiated PROCEDURE_REF target: " + to_string(procedure_ref.functanoid));
                            }

                            enqueue_functanoid(
                                caller,
                                instanciation_reference{
                                    .temploid = selection,
                                    .params = instantiate_declared_overload(*formal_ensig),
                                },
                                std::nullopt,
                                traceback);
                            continue;
                        }

                        initialization_reference procedure_call;
                        if (resolved_procedure->type_is< initialization_reference >())
                        {
                            procedure_call = resolved_procedure->get_as< initialization_reference >();
                        }
                        else
                        {
                            procedure_call.initializee = *resolved_procedure;
                        }

                        rpnx::querygraph::request< instanciation_query > instanciation_request(procedure_call);
                        co_yield rpnx::querygraph::dependency(instanciation_request);

                        std::optional< instanciation_reference > const instanciation = co_await instanciation_request;
                        if (!instanciation.has_value())
                        {
                            throw semantic_compilation_error("PROCEDURE_REF target is not callable as a concrete function: " + to_string(procedure_ref.functanoid));
                        }
                        enqueue_functanoid(caller, *instanciation, std::nullopt, traceback);
                    }
                }
            }
        }
    };

    auto enqueue_asm_object_lookup_requests =
        [](std::vector< std::pair< type_symbol, rpnx::querygraph::request< lookup_query > > >& lookup_requests,
            type_symbol const& context,
            ast2_asm_procedure_declaration const& routine) -> void
    {
        for (ast2_asm_instruction const& instruction : routine.instructions)
        {
            for (ast2_asm_operand const& operand : instruction.operands)
            {
                for (ast2_asm_operand_component const& component : operand.components)
                {
                    if (component.type_is< ast2_object_ref >())
                    {
                        type_symbol const object_reference = component.get_as< ast2_object_ref >().object;
                        lookup_requests.push_back(std::make_pair(
                            object_reference,
                            rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                                .context = context,
                                .type = object_reference,
                            })));
                    }
                }
            }
        }
    };

    auto await_object_lookup_requests =
        [&](std::vector< std::pair< type_symbol, rpnx::querygraph::request< lookup_query > > >& lookup_requests) -> rpnx::querygraph::cosubroutine_impl< output_llvm_input_spec, void >
    {
        for (std::pair< type_symbol, rpnx::querygraph::request< lookup_query > >& lookup_request : lookup_requests)
        {
            std::optional< type_symbol > const canonical_object = co_await lookup_request.second;
            if (!canonical_object.has_value())
            {
                throw semantic_compilation_error("OBJECT_REF target could not be resolved: " + to_string(lookup_request.first));
            }
            object_references.insert(*canonical_object);
        }
    };

    enqueue_vmir_references(output_module_unit.target_name, output_module_unit.target_code, {});
    for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine_entry : output_module_unit.inlinable_functions)
    {
        enqueue_vmir_references(routine_entry.first, routine_entry.second, {});
    }

    if (runtime_program_start_request.has_value())
    {
        ast2_symboid const& runtime_symboid = co_await *runtime_program_start_request;
        if (!runtime_symboid.type_is< std::monostate >())
        {
            if (!runtime_symboid.type_is< ast2_asm_procedure_declaration >())
            {
                throw semantic_compilation_error("RUNTIME::" + selected_runtime_start_name + " must be an ASM_PROCEDURE");
            }
            runtime_program_start = *runtime_program_start_candidate;
            if ((output_info.type == output_kind::executable || output_info.type == output_kind::unit_test_suite) && machine.os_type == os::linux && machine.binary_type == binary::elf)
            {
                output_module_unit.executable_entry_symbol = to_string(*runtime_program_start);
            }
        }
    }
    else if (output_info.type == output_kind::unit_test_suite)
    {
        throw semantic_compilation_error("unit_test_suite output requires MODULE(RUNTIME)::UNIT_TESTING_PROGRAM_START");
    }

    if (output_info.type == output_kind::unit_test_suite && !runtime_program_start.has_value())
    {
        throw semantic_compilation_error("unit_test_suite output requires MODULE(RUNTIME)::UNIT_TESTING_PROGRAM_START");
    }

    if (runtime_program_start.has_value())
    {
        rpnx::querygraph::request< asm_procedure_from_symbol_query > runtime_asm_request(*runtime_program_start);
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< lookup_query > > > runtime_object_lookup_requests;

        co_yield rpnx::querygraph::dependency(runtime_asm_request);

        ast2_symboid const& runtime_symboid = co_await *runtime_program_start_request;
        ast2_asm_procedure_declaration const& runtime_declaration = runtime_symboid.get_as< ast2_asm_procedure_declaration >();
        co_await co_enqueue_asm_references(*runtime_program_start, runtime_declaration, {});
        enqueue_asm_object_lookup_requests(runtime_object_lookup_requests, *runtime_program_start, runtime_declaration);
        for (std::pair< type_symbol, rpnx::querygraph::request< lookup_query > >& lookup_request : runtime_object_lookup_requests)
        {
            co_yield rpnx::querygraph::dependency(lookup_request.second);
        }

        output_module_unit.asm_functions.emplace(*runtime_program_start, co_await runtime_asm_request);
        co_await await_object_lookup_requests(runtime_object_lookup_requests);
    }

    while (!pending_functanoids.empty() || !pending_runtime_procedures.empty())
    {
        while (!pending_runtime_procedures.empty())
        {
            std::vector< llvm_backend::runtime_procedure_reference > round_runtime_procedures;
            while (!pending_runtime_procedures.empty())
            {
                round_runtime_procedures.push_back(*pending_runtime_procedures.begin());
                pending_runtime_procedures.erase(pending_runtime_procedures.begin());
            }

            std::vector< std::pair< llvm_backend::runtime_procedure_reference, rpnx::querygraph::request< lookup_query > > > runtime_lookup_requests;
            for (llvm_backend::runtime_procedure_reference const& runtime_reference : round_runtime_procedures)
            {
                if (output_module_unit.runtime_procedures.contains(runtime_reference))
                {
                    continue;
                }
                if (!target_config.module_configurations.contains("RUNTIME"))
                {
                    throw semantic_compilation_error("Native ASSERT lowering requires MODULE(RUNTIME)::ASSERT_FAIL");
                }
                initialization_reference runtime_init = llvm_backend::runtime_procedure_initialization(runtime_reference.procedure);
                runtime_init.context = absolute_module_reference{.module_name = "RUNTIME"};
                runtime_lookup_requests.push_back(std::make_pair(runtime_reference, rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                                                                                     .context = absolute_module_reference{.module_name = "RUNTIME"},
                                                                                     .type = std::move(runtime_init),
                                                                                 })));
                co_yield rpnx::querygraph::dependency(runtime_lookup_requests.back().second);
            }

            std::vector< std::pair< llvm_backend::runtime_procedure_reference, rpnx::querygraph::request< instanciation_query > > > runtime_instanciation_requests;
            for (std::pair< llvm_backend::runtime_procedure_reference, rpnx::querygraph::request< lookup_query > >& request : runtime_lookup_requests)
            {
                std::optional< type_symbol > const runtime_lookup = co_await request.second;
                if (!runtime_lookup.has_value())
                {
                    throw semantic_compilation_error("Could not resolve runtime procedure: " + to_string(llvm_backend::runtime_procedure_initializee(request.first.procedure)));
                }
                if (runtime_lookup->type_is< instanciation_reference >())
                {
                    output_module_unit.runtime_procedures.emplace(request.first, *runtime_lookup);
                    enqueue_functanoid(
                        llvm_backend::runtime_procedure_initializee(request.first.procedure),
                        *runtime_lookup,
                        std::nullopt,
                        {});
                    continue;
                }
                if (!runtime_lookup->type_is< initialization_reference >())
                {
                    throw semantic_compilation_error("Runtime procedure did not resolve to an initialization reference: " + to_string(llvm_backend::runtime_procedure_initializee(request.first.procedure)));
                }
                runtime_instanciation_requests.push_back(std::make_pair(request.first, rpnx::querygraph::request< instanciation_query >(runtime_lookup->as< initialization_reference >())));
                co_yield rpnx::querygraph::dependency(runtime_instanciation_requests.back().second);
            }

            for (std::pair< llvm_backend::runtime_procedure_reference, rpnx::querygraph::request< instanciation_query > >& request : runtime_instanciation_requests)
            {
                std::optional< instanciation_reference > const runtime_instanciation = co_await request.second;
                if (!runtime_instanciation.has_value())
                {
                    throw semantic_compilation_error("Could not initialize runtime procedure: " + to_string(llvm_backend::runtime_procedure_initializee(request.first.procedure)));
                }
                type_symbol const runtime_symbol = *runtime_instanciation;
                output_module_unit.runtime_procedures.emplace(request.first, runtime_symbol);
                enqueue_functanoid(
                    llvm_backend::runtime_procedure_initializee(request.first.procedure),
                    runtime_symbol,
                    std::nullopt,
                    {});
            }
        }

        if (pending_functanoids.empty())
        {
            continue;
        }

        std::vector< std::pair< instanciation_reference, std::vector< trace_frame > > > round_functanoids;
        while (!pending_functanoids.empty())
        {
            std::pair< instanciation_reference, std::vector< trace_frame > > pending_functanoid = std::move(pending_functanoids.back());
            pending_functanoids.pop_back();

            type_symbol const functanoid_symbol = type_symbol(pending_functanoid.first);
            if ((entry_functanoid_symbol.has_value() && functanoid_symbol == *entry_functanoid_symbol) || output_module_unit.inlinable_functions.contains(functanoid_symbol) || output_module_unit.asm_callable_interfaces.contains(functanoid_symbol))
            {
                continue;
            }
            round_functanoids.push_back(std::move(pending_functanoid));
        }

        std::vector< std::pair< type_symbol, rpnx::querygraph::request< symboid_query > > > symboid_requests;
        symboid_requests.reserve(round_functanoids.size());
        for (std::pair< instanciation_reference, std::vector< trace_frame > > const& round_functanoid : round_functanoids)
        {
            type_symbol const functanoid_symbol = type_symbol(round_functanoid.first);
            type_symbol asm_declaration_symbol = functanoid_symbol;
            if (functanoid_symbol.type_is< instanciation_reference >())
            {
                asm_declaration_symbol = functanoid_symbol.get_as< instanciation_reference >().temploid.templexoid;
            }
            symboid_requests.push_back(std::make_pair(functanoid_symbol, rpnx::querygraph::request< symboid_query >(asm_declaration_symbol)));
            co_yield rpnx::querygraph::dependency(symboid_requests.back().second);
        }

        std::vector< std::pair< type_symbol, rpnx::querygraph::request< asm_procedure_from_symbol_query > > > asm_body_requests;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< asm_procedure_from_symbol_query > > > asm_callable_requests;
        std::set< type_symbol > requested_asm_bodies;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< vm_procedure3_query > > > vm_requests;
        std::vector< std::vector< trace_frame > > vm_tracebacks;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< lookup_query > > > object_lookup_requests;

        for (std::size_t i = 0; i < round_functanoids.size(); ++i)
        {
            type_symbol const functanoid_symbol = type_symbol(round_functanoids.at(i).first);
            type_symbol asm_declaration_symbol = functanoid_symbol;
            if (functanoid_symbol.type_is< instanciation_reference >())
            {
                asm_declaration_symbol = functanoid_symbol.get_as< instanciation_reference >().temploid.templexoid;
            }
            ast2_symboid const& symboid = co_await symboid_requests.at(i).second;
            if (symboid.type_is< ast2_asm_procedure_declaration >())
            {
                ast2_asm_procedure_declaration const& declaration = symboid.get_as< ast2_asm_procedure_declaration >();
                if (!output_module_unit.asm_functions.contains(asm_declaration_symbol) && requested_asm_bodies.insert(asm_declaration_symbol).second)
                {
                    asm_body_requests.push_back(std::make_pair(asm_declaration_symbol, rpnx::querygraph::request< asm_procedure_from_symbol_query >(asm_declaration_symbol)));
                    co_yield rpnx::querygraph::dependency(asm_body_requests.back().second);
                }
                asm_callable_requests.push_back(std::make_pair(functanoid_symbol, rpnx::querygraph::request< asm_procedure_from_symbol_query >(functanoid_symbol)));
                co_yield rpnx::querygraph::dependency(asm_callable_requests.back().second);
                co_await co_enqueue_asm_references(asm_declaration_symbol, declaration, round_functanoids.at(i).second);
                enqueue_asm_object_lookup_requests(object_lookup_requests, asm_declaration_symbol, declaration);
                continue;
            }
            else if (symboid.type_is< ast2_extern_procedure >())
            {
                ast2_extern_procedure const& declaration = symboid.get_as< ast2_extern_procedure >();
                output_module_unit.extern_procedures.insert(functanoid_symbol);
                if (declaration.is_optional)
                {
                    output_module_unit.optional_extern_procedures.insert(functanoid_symbol);
                }
                if (declaration.version)
                {
                    output_module_unit.extern_procedure_versions.emplace(functanoid_symbol, *declaration.version);
                }
                asm_callable_requests.push_back(std::make_pair(functanoid_symbol, rpnx::querygraph::request< asm_procedure_from_symbol_query >(functanoid_symbol)));
                co_yield rpnx::querygraph::dependency(asm_callable_requests.back().second);
                continue;
            }

            vm_requests.push_back(std::make_pair(functanoid_symbol, rpnx::querygraph::request< vm_procedure3_query >(round_functanoids.at(i).first)));
            vm_tracebacks.push_back(round_functanoids.at(i).second);
            co_yield rpnx::querygraph::dependency(vm_requests.back().second);
        }

        for (std::pair< type_symbol, rpnx::querygraph::request< lookup_query > >& lookup_request : object_lookup_requests)
        {
            co_yield rpnx::querygraph::dependency(lookup_request.second);
        }

        for (std::pair< type_symbol, rpnx::querygraph::request< asm_procedure_from_symbol_query > >& asm_request : asm_body_requests)
        {
            output_module_unit.asm_functions.emplace(asm_request.first, co_await asm_request.second);
        }
        for (std::pair< type_symbol, rpnx::querygraph::request< asm_procedure_from_symbol_query > >& asm_request : asm_callable_requests)
        {
            asm_procedure const selected_procedure = co_await asm_request.second;
            if (!selected_procedure.callable_interface.has_value())
            {
                throw compiler_bug("Selected asm procedure has no callable interface: " + to_string(asm_request.first));
            }
            output_module_unit.asm_callable_interfaces.emplace(asm_request.first, *selected_procedure.callable_interface);
        }
        co_await await_object_lookup_requests(object_lookup_requests);

        for (std::size_t i = 0; i < vm_requests.size(); ++i)
        {
            vmir2::functanoid_routine3 procedure;
            try
            {
                procedure = co_await vm_requests.at(i).second;
            }
            catch (compilation_error& error)
            {
                error.traceback.insert(error.traceback.end(), vm_tracebacks.at(i).begin(), vm_tracebacks.at(i).end());
                throw;
            }

            type_symbol const functanoid_symbol = vm_requests.at(i).first;
            enqueue_vmir_references(functanoid_symbol, procedure, vm_tracebacks.at(i));
            output_module_unit.inlinable_functions.emplace(functanoid_symbol, std::move(procedure));
        }
    }

    std::set< type_symbol > seen_types;
    std::vector< type_symbol > pending_types;
    std::set< type_symbol > seen_antestatal_globals;
    std::vector< type_symbol > pending_antestatal_globals;
    std::set< type_symbol > global_init_roots;

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

    auto enqueue_asm_callable_surface_types = [&](asm_callable const& callable) -> void
    {
        for (asm_argument_binding const& arg : callable.args)
        {
            enqueue_type(arg.type);
        }
        if (callable.return_type.has_value())
        {
            enqueue_type(*callable.return_type);
        }
    };

    auto enqueue_routine_support_roots = [&](vmir2::functanoid_routine3 const& routine) -> void
    {
        for (type_symbol const& placement_root : vmir2::directly_required_type_placements(routine))
        {
            enqueue_type(placement_root);
        }
        for (type_symbol const& antestatal_root : vmir2::directly_referenced_antestatal_globals(routine))
        {
            enqueue_antestatal_global(antestatal_root);
        }
        for (type_symbol const& global_root : vmir2::directly_referenced_global_roots(routine))
        {
            global_init_roots.insert(global_root);
        }
        for (std::pair< static_snapshot_ref const, vmir2::localdata_entry > const& snapshot_entry : routine.static_snapshots)
        {
            type_symbol const snapshot_symbol = type_symbol(snapshot_entry.first);
            output_module_unit.antestatal_constants[snapshot_symbol] = snapshot_entry.second.value;
            enqueue_type(snapshot_entry.second.type);
        }
    };

    enqueue_routine_support_roots(output_module_unit.target_code);
    for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine_entry : output_module_unit.inlinable_functions)
    {
        enqueue_routine_support_roots(routine_entry.second);
    }
    for (std::pair< type_symbol const, asm_callable > const& callable_entry : output_module_unit.asm_callable_interfaces)
    {
        enqueue_asm_callable_surface_types(callable_entry.second);
    }

    std::vector< std::pair< type_symbol, rpnx::querygraph::request< symbol_type_query > > > object_kind_requests;
    if (output_info.type == output_kind::unit_test_suite)
    {
        object_references.insert(builtin_symbol{.name = "UNIT_TEST_COUNT"});
        object_references.insert(builtin_symbol{.name = "UNIT_TEST_NAMES"});
        object_references.insert(builtin_symbol{.name = "UNIT_TEST_PROC"});
    }
    for (type_symbol const& object_reference : object_references)
    {
        if (is_main_function_object_symbol(object_reference))
        {
            type_symbol const object_type = main_function_object_type();
            output_module_unit.object_reference_types.emplace(object_reference, object_type);
            enqueue_type(object_type);
            continue;
        }
        if (std::optional< type_symbol > const unit_test_object_type = llvm_backend::unit_test_object_type(object_reference); unit_test_object_type.has_value())
        {
            output_module_unit.object_reference_types.emplace(object_reference, *unit_test_object_type);
            enqueue_type(*unit_test_object_type);
            continue;
        }
        object_kind_requests.push_back(std::make_pair(object_reference, rpnx::querygraph::request< symbol_type_query >(object_reference)));
        co_yield rpnx::querygraph::dependency(object_kind_requests.back().second);
    }

    std::vector< type_symbol > global_object_references;
    for (std::pair< type_symbol, rpnx::querygraph::request< symbol_type_query > >& object_kind_request : object_kind_requests)
    {
        symbol_kind const kind = co_await object_kind_request.second;
        if (kind != symbol_kind::global_variable)
        {
            throw semantic_compilation_error("OBJECT_REF target is not a global object: " + to_string(object_kind_request.first));
        }
        global_object_references.push_back(object_kind_request.first);
        global_init_roots.insert(object_kind_request.first);
    }

    std::vector< std::pair< type_symbol, rpnx::querygraph::request< variable_type_query > > > object_type_requests;
    std::vector< std::pair< type_symbol, rpnx::querygraph::request< global_is_antestatal_static_query > > > object_static_requests;
    for (type_symbol const& object_reference : global_object_references)
    {
        object_type_requests.push_back(std::make_pair(object_reference, rpnx::querygraph::request< variable_type_query >(object_reference)));
        object_static_requests.push_back(std::make_pair(object_reference, rpnx::querygraph::request< global_is_antestatal_static_query >(object_reference)));
        co_yield rpnx::querygraph::dependency(object_type_requests.back().second);
        co_yield rpnx::querygraph::dependency(object_static_requests.back().second);
    }
    for (std::pair< type_symbol, rpnx::querygraph::request< variable_type_query > >& object_type_request : object_type_requests)
    {
        type_symbol const object_type = co_await object_type_request.second;
        output_module_unit.object_reference_types.emplace(object_type_request.first, object_type);
        enqueue_type(object_type);
    }
    for (std::pair< type_symbol, rpnx::querygraph::request< global_is_antestatal_static_query > >& object_static_request : object_static_requests)
    {
        if (co_await object_static_request.second)
        {
            enqueue_antestatal_global(object_static_request.first);
        }
    }

    std::vector< std::pair< type_symbol, rpnx::querygraph::request< global_init_type_query > > > global_init_requests;
    for (type_symbol const& global_root : global_init_roots)
    {
        global_init_requests.push_back(std::make_pair(global_root, rpnx::querygraph::request< global_init_type_query >(global_root)));
        co_yield rpnx::querygraph::dependency(global_init_requests.back().second);
    }
    for (std::pair< type_symbol, rpnx::querygraph::request< global_init_type_query > >& global_init_request : global_init_requests)
    {
        output_module_unit.global_init_types[global_init_request.first] = co_await global_init_request.second;
    }

    while (!pending_antestatal_globals.empty())
    {
        std::vector< type_symbol > round_antestatal_globals;
        while (!pending_antestatal_globals.empty())
        {
            round_antestatal_globals.push_back(std::move(pending_antestatal_globals.back()));
            pending_antestatal_globals.pop_back();
        }

        std::vector< std::pair< type_symbol, rpnx::querygraph::request< global_is_antestatal_static_query > > > static_requests;
        for (type_symbol const& symbol : round_antestatal_globals)
        {
            static_requests.push_back(std::make_pair(symbol, rpnx::querygraph::request< global_is_antestatal_static_query >(symbol)));
            co_yield rpnx::querygraph::dependency(static_requests.back().second);
        }

        std::vector< type_symbol > static_globals;
        for (std::pair< type_symbol, rpnx::querygraph::request< global_is_antestatal_static_query > >& static_request : static_requests)
        {
            if (co_await static_request.second)
            {
                static_globals.push_back(static_request.first);
            }
        }

        std::vector< std::pair< type_symbol, rpnx::querygraph::request< antestatal_static_value_query > > > value_requests;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< variable_type_query > > > type_requests;
        for (type_symbol const& symbol : static_globals)
        {
            value_requests.push_back(std::make_pair(symbol, rpnx::querygraph::request< antestatal_static_value_query >(symbol)));
            type_requests.push_back(std::make_pair(symbol, rpnx::querygraph::request< variable_type_query >(symbol)));
            co_yield rpnx::querygraph::dependency(value_requests.back().second);
            co_yield rpnx::querygraph::dependency(type_requests.back().second);
        }
        for (std::pair< type_symbol, rpnx::querygraph::request< antestatal_static_value_query > >& value_request : value_requests)
        {
            output_module_unit.antestatal_constants[value_request.first] = co_await value_request.second;
        }
        for (std::pair< type_symbol, rpnx::querygraph::request< variable_type_query > >& type_request : type_requests)
        {
            enqueue_type(co_await type_request.second);
        }
    }

    while (!pending_types.empty())
    {
        std::vector< type_symbol > round_types;
        while (!pending_types.empty())
        {
            round_types.push_back(std::move(pending_types.back()));
            pending_types.pop_back();
        }

        std::vector< std::pair< type_symbol, rpnx::querygraph::request< type_placement_info_query > > > type_placement_requests;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< symbol_type_query > > > symbol_type_requests;
        for (type_symbol const& type : round_types)
        {
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
                output_module_unit.type_placements[type] = type_placement_info{
                    .size = machine.pointer_size_bytes(),
                    .alignment = machine.pointer_align(),
                };
            }
            else if (!skip_type_placement_query)
            {
                type_placement_requests.push_back(std::make_pair(type, rpnx::querygraph::request< type_placement_info_query >(type)));
                co_yield rpnx::querygraph::dependency(type_placement_requests.back().second);
            }

            if (type.type_is< subsymbol >() || type.type_is< instanciation_reference >() || type.type_is< readonly_constant >())
            {
                symbol_type_requests.push_back(std::make_pair(type, rpnx::querygraph::request< symbol_type_query >(type)));
                co_yield rpnx::querygraph::dependency(symbol_type_requests.back().second);
            }
        }

        for (std::pair< type_symbol, rpnx::querygraph::request< type_placement_info_query > >& type_placement_request : type_placement_requests)
        {
            output_module_unit.type_placements[type_placement_request.first] = co_await type_placement_request.second;
        }

        std::vector< type_symbol > interface_types;
        std::vector< type_symbol > enum_types;
        std::vector< type_symbol > flagset_types;
        std::vector< type_symbol > class_types;
        for (std::pair< type_symbol, rpnx::querygraph::request< symbol_type_query > >& symbol_type_request : symbol_type_requests)
        {
            symbol_kind const kind = co_await symbol_type_request.second;
            if (kind == symbol_kind::interface_)
            {
                interface_types.push_back(symbol_type_request.first);
                continue;
            }
            if (kind == symbol_kind::enum_)
            {
                enum_types.push_back(symbol_type_request.first);
                continue;
            }
            if (kind == symbol_kind::flagset_)
            {
                flagset_types.push_back(symbol_type_request.first);
                continue;
            }
            class_types.push_back(symbol_type_request.first);
        }

        std::vector< std::pair< type_symbol, rpnx::querygraph::request< interface_slot_list_query > > > interface_slot_requests;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< enum_info_query > > > enum_info_requests;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< flagset_info_query > > > flagset_info_requests;
        std::vector< std::pair< type_symbol, rpnx::querygraph::request< class_layout_query > > > class_layout_requests;
        for (type_symbol const& type : interface_types)
        {
            interface_slot_requests.push_back(std::make_pair(type, rpnx::querygraph::request< interface_slot_list_query >(type)));
            co_yield rpnx::querygraph::dependency(interface_slot_requests.back().second);
        }
        for (type_symbol const& type : enum_types)
        {
            enum_info_requests.push_back(std::make_pair(type, rpnx::querygraph::request< enum_info_query >(type)));
            co_yield rpnx::querygraph::dependency(enum_info_requests.back().second);
        }
        for (type_symbol const& type : flagset_types)
        {
            flagset_info_requests.push_back(std::make_pair(type, rpnx::querygraph::request< flagset_info_query >(type)));
            co_yield rpnx::querygraph::dependency(flagset_info_requests.back().second);
        }
        for (type_symbol const& type : class_types)
        {
            class_layout_requests.push_back(std::make_pair(type, rpnx::querygraph::request< class_layout_query >(type)));
            co_yield rpnx::querygraph::dependency(class_layout_requests.back().second);
        }

        for (std::pair< type_symbol, rpnx::querygraph::request< interface_slot_list_query > >& interface_slot_request : interface_slot_requests)
        {
            std::vector< interface_slot > const slots = co_await interface_slot_request.second;
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
            output_module_unit.interface_slots[interface_slot_request.first] = std::move(slot_keys);
        }
        for (std::pair< type_symbol, rpnx::querygraph::request< enum_info_query > >& enum_info_request : enum_info_requests)
        {
            output_module_unit.enum_infos[enum_info_request.first] = co_await enum_info_request.second;
        }
        for (std::pair< type_symbol, rpnx::querygraph::request< flagset_info_query > >& flagset_info_request : flagset_info_requests)
        {
            output_module_unit.flagset_infos[flagset_info_request.first] = co_await flagset_info_request.second;
        }
        for (std::pair< type_symbol, rpnx::querygraph::request< class_layout_query > >& class_layout_request : class_layout_requests)
        {
            class_layout const layout = co_await class_layout_request.second;
            output_module_unit.class_layouts[class_layout_request.first] = layout;
            for (class_field_info const& field : layout.fields)
            {
                enqueue_type(field.type);
            }
        }
    }

    std::vector< std::pair< type_symbol, rpnx::querygraph::request< procedure_linksymbol_query > > > procedure_linksymbol_requests;
    if (entry_functanoid_symbol.has_value())
    {
        procedure_linksymbol_requests.push_back(std::make_pair(
            *entry_functanoid_symbol,
            rpnx::querygraph::request< procedure_linksymbol_query >(ast2_procedure_ref{.cc = "", .functanoid = *entry_functanoid_symbol})));
    }
    for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine_entry : output_module_unit.inlinable_functions)
    {
        procedure_linksymbol_requests.push_back(std::make_pair(
            routine_entry.first,
            rpnx::querygraph::request< procedure_linksymbol_query >(ast2_procedure_ref{.cc = "", .functanoid = routine_entry.first})));
    }
    for (std::pair< type_symbol const, asm_procedure > const& routine_entry : output_module_unit.asm_functions)
    {
        procedure_linksymbol_requests.push_back(std::make_pair(
            routine_entry.first,
            rpnx::querygraph::request< procedure_linksymbol_query >(ast2_procedure_ref{.cc = "", .functanoid = routine_entry.first})));
    }
    for (std::pair< type_symbol const, asm_callable > const& routine_entry : output_module_unit.asm_callable_interfaces)
    {
        type_symbol asm_declaration_symbol = routine_entry.first;
        if (routine_entry.first.type_is< instanciation_reference >())
        {
            asm_declaration_symbol = routine_entry.first.get_as< instanciation_reference >().temploid.templexoid;
        }
        procedure_linksymbol_requests.push_back(std::make_pair(
            routine_entry.first,
            rpnx::querygraph::request< procedure_linksymbol_query >(ast2_procedure_ref{.cc = "", .functanoid = asm_declaration_symbol})));
    }
    for (std::pair< llvm_backend::runtime_procedure_reference const, type_symbol > const& routine_entry : output_module_unit.runtime_procedures)
    {
        procedure_linksymbol_requests.push_back(std::make_pair(
            routine_entry.second,
            rpnx::querygraph::request< procedure_linksymbol_query >(ast2_procedure_ref{.cc = "", .functanoid = routine_entry.second})));
    }
    for (std::pair< type_symbol, rpnx::querygraph::request< procedure_linksymbol_query > >& linksymbol_request : procedure_linksymbol_requests)
    {
        co_yield rpnx::querygraph::dependency(linksymbol_request.second);
    }
    for (std::pair< type_symbol, rpnx::querygraph::request< procedure_linksymbol_query > >& linksymbol_request : procedure_linksymbol_requests)
    {
        output_module_unit.procedure_linksymbols.emplace(linksymbol_request.first, co_await linksymbol_request.second);
    }

    output_module_unit.source_index = rpnx::cow< vmir2::source_index >(source_index);

    co_return output_module_unit;
}
