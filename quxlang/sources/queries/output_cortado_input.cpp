// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/output_cortado_input_spec.hpp>

#include <rpnx/unimplemented.hpp>

#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::output_cortado_input_spec > quxlang::output_cortado_input_impl(std::string input)
{
    output_query_output const& output_info = co_await rpnx::querygraph::request< output_binary_information_query >(input);
    target_configuration const& target = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    backend_cortado_options const& options = co_await rpnx::querygraph::request< output_cortado_backend_options_query >(input);
    vmir2::source_index const& source_index = co_await rpnx::querygraph::request< indexed_source_bundle_query >(std::monostate{});

    if (target.backend != backend_kind::cortado || target.target_output_config.cpu_type != cpu::jvm)
    {
        throw semantic_compilation_error("Quxlang's Cortado backend input requires a JVM target");
    }
    if (output_info.type != output_kind::executable && output_info.type != output_kind::unit_test_suite)
    {
        throw rpnx::unimplemented();
    }
    for (std::string const& module_name : output_info.module_names)
    {
        if (!target.module_configurations.contains(module_name))
        {
            throw semantic_compilation_error("Output '" + output_info.output_name + "' references unknown module '" + module_name + "'");
        }
    }

    cortado_backend::cortado_compilable_unit result{
        .output_name = output_info.output_name,
        .kind = output_info.type,
        .options = options,
    };
    result.source_index = rpnx::cow< vmir2::source_index >(source_index);

    auto valid_jvm_internal_name = [](std::string const& name) -> bool
    {
        if (name.empty() || name.front() == '/' || name.back() == '/')
        {
            return false;
        }
        bool previous_was_slash = false;
        for (char character : name)
        {
            if (character == '.' || character == ':' || character == '[' || character == ';')
            {
                return false;
            }
            if (character == '/')
            {
                if (previous_was_slash)
                {
                    return false;
                }
                previous_was_slash = true;
            }
            else
            {
                previous_was_slash = false;
            }
        }
        return true;
    };

    auto jvm_descriptor_for_type = [&](type_symbol type) -> std::string
    {
        while (type.type_is< nvalue_slot >() || type.type_is< dvalue_slot >())
        {
            type = type.type_is< nvalue_slot >() ? type.get_as< nvalue_slot >().target : type.get_as< dvalue_slot >().target;
        }
        if (type.type_is< void_type >())
        {
            return "V";
        }
        if (type.type_is< bool_type >())
        {
            return "Z";
        }
        if (type.type_is< byte_type >())
        {
            return "B";
        }
        if (type.type_is< int_type >())
        {
            int_type const& integer = type.get_as< int_type >();
            if (!integer.has_sign)
            {
                throw semantic_compilation_error("JVM external bindings do not support unsigned integer ABI types: " + to_string(type));
            }
            if (integer.bits == 8)
            {
                return "B";
            }
            if (integer.bits == 16)
            {
                return "S";
            }
            if (integer.bits == 32)
            {
                return "I";
            }
            if (integer.bits == 64)
            {
                return "J";
            }
            throw semantic_compilation_error("JVM external bindings support only I8, I16, I32, and I64 integer ABI types: " + to_string(type));
        }
        if (type.type_is< float_type >())
        {
            float_type const& floating = type.get_as< float_type >();
            if (floating.bits == 32 && floating.exponent_bits == 8)
            {
                return "F";
            }
            if (floating.bits == 64 && floating.exponent_bits == 11)
            {
                return "D";
            }
            throw semantic_compilation_error("JVM external bindings support only F32 and F64 floating-point ABI types: " + to_string(type));
        }
        if (type.type_is< ptrref_type >() && type.get_as< ptrref_type >().ptr_class == pointer_class::gc)
        {
            ptrref_type const& pointer = type.get_as< ptrref_type >();
            std::map< type_symbol, cortado_backend::jvm_external_type_info >::const_iterator const external = result.external_types.find(pointer.target);
            if (external == result.external_types.end())
            {
                throw compiler_bug("Quxlang's Cortado backend JVM ABI type was not resolved before descriptor generation: " + to_string(type));
            }
            return "L" + external->second.internal_name + ";";
        }
        throw semantic_compilation_error("Unsupported Quxlang type at reached JVM external binding boundary: " + to_string(type));
    };

    std::set< type_symbol > queued_routines;
    std::vector< instanciation_reference > pending_routines;
    std::set< type_symbol > semantic_type_roots;
    std::set< type_symbol > global_roots;
    std::set< type_symbol > queued_antestatal_globals;
    std::vector< type_symbol > pending_antestatal_globals;

    auto enqueue_routine = [&](type_symbol const& symbol) -> void
    {
        if (!symbol.type_is< instanciation_reference >())
        {
            throw compiler_bug("Quxlang's Cortado backend dependency scan returned a non-instanciation reference: " + to_string(symbol));
        }
        if (queued_routines.insert(symbol).second)
        {
            pending_routines.push_back(symbol.as< instanciation_reference >());
        }
    };

    auto enqueue_antestatal_global = [&](type_symbol const& symbol) -> void
    {
        global_roots.insert(symbol);
        if (queued_antestatal_globals.insert(symbol).second)
        {
            pending_antestatal_globals.push_back(symbol);
        }
    };

    auto collect_dependency_references = [&](dependencies const& direct) -> void
    {
        for (std::pair< type_symbol const, std::optional< source_location > > const& referenced : direct.functanoids)
        {
            enqueue_routine(referenced.first);
        }
        result.runtime_requirements.insert(direct.runtime_dependencies.begin(), direct.runtime_dependencies.end());
        semantic_type_roots.insert(direct.type_placements.begin(), direct.type_placements.end());
        semantic_type_roots.insert(direct.struct_layouts.begin(), direct.struct_layouts.end());
        semantic_type_roots.insert(direct.fusion_layouts.begin(), direct.fusion_layouts.end());
        global_roots.insert(direct.global_roots.begin(), direct.global_roots.end());
        for (type_symbol const& global : direct.antestatal_globals)
        {
            enqueue_antestatal_global(global);
        }
    };

    auto collect_dependencies = [&](type_symbol const& owner, vmir2::functanoid_routine3 const& routine, dependencies const& direct) -> void
    {
        collect_dependency_references(direct);
        for (static_snapshot_ref const& snapshot : direct.static_snapshots)
        {
            std::map< static_snapshot_ref, vmir2::localdata_entry >::const_iterator const entry = routine.static_snapshots.find(snapshot);
            if (entry == routine.static_snapshots.end())
            {
                throw compiler_bug("Missing Quxlang Cortado backend static snapshot for " + to_string(owner));
            }
            type_symbol const snapshot_symbol = snapshot;
            result.global_types.emplace(snapshot_symbol, entry->second.type);
            result.global_values.emplace(snapshot_symbol, entry->second.value);
            semantic_type_roots.insert(entry->second.type);
        }
    };

    auto runtime_procedure_name = [](vmir_runtime_dependency dependency) -> std::string
    {
        switch (dependency)
        {
        case vmir_runtime_dependency::assert_fail:
            return "ASSERT_FAIL";
        case vmir_runtime_dependency::panic:
            return "PANIC";
        case vmir_runtime_dependency::initguard_complete:
            return "INITGUARD_COMPLETE";
        case vmir_runtime_dependency::initguard_abort:
            return "INITGUARD_ABORT";
        case vmir_runtime_dependency::initguard_try_acquire:
            return "INITGUARD_TRY_ACQUIRE";
        }
        throw compiler_bug("Unknown Quxlang Cortado backend runtime dependency");
    };

    auto runtime_procedure_initialization = [&](vmir_runtime_dependency dependency) -> initialization_reference
    {
        type_symbol const runtime_context = absolute_module_reference{.module_name = "RUNTIME"};
        initialization_reference initialization{
            .initializee =
                subsymbol{
                    .of = runtime_context,
                    .name = runtime_procedure_name(dependency),
                },
            .context = runtime_context,
            .adaptations = allowed_adaptations::none,
        };
        if (dependency == vmir_runtime_dependency::assert_fail)
        {
            type_symbol const string_type = readonly_constant{.kind = constant_kind::string};
            initialization.parameters.named["expr"] = make_type_instantiation(string_type);
            initialization.parameters.named["file"] = make_type_instantiation(size_type{});
            initialization.parameters.named["line"] = make_type_instantiation(size_type{});
            initialization.parameters.named["column"] = make_type_instantiation(size_type{});
            initialization.parameters.named["tag"] = make_type_instantiation(ptrref_type{
                .target = string_type,
                .ptr_class = pointer_class::instance,
                .qual = qualifier::constant,
            });
        }
        else if (dependency == vmir_runtime_dependency::panic)
        {
            type_symbol const string_type = readonly_constant{.kind = constant_kind::string};
            initialization.parameters.named["message"] = make_type_instantiation(string_type);
            initialization.parameters.named["file"] = make_type_instantiation(size_type{});
            initialization.parameters.named["line"] = make_type_instantiation(size_type{});
            initialization.parameters.named["column"] = make_type_instantiation(size_type{});
        }
        else
        {
            initialization.parameters.named["guard"] = make_type_instantiation(ptrref_type{
                .target = initguard_type{},
                .ptr_class = pointer_class::ref,
                .qual = qualifier::mut,
            });
        }
        return initialization;
    };

    if (output_info.type == output_kind::executable)
    {
        if (!output_info.main_functanoid.has_value())
        {
            throw semantic_compilation_error("Output '" + output_info.output_name + "' requires a main functanoid");
        }

        type_symbol const module_context = absolute_module_reference{.module_name = output_info.module_names.front()};
        type_symbol const contextual_entry = with_context(*output_info.main_functanoid, module_context);
        std::optional< type_symbol > const resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = module_context,
            .type = contextual_entry,
        });
        if (!resolved.has_value())
        {
            throw semantic_compilation_error("Could not resolve the Quxlang Cortado backend entry functanoid " + to_string(*output_info.main_functanoid));
        }

        std::optional< instanciation_reference > entry;
        if (resolved->type_is< instanciation_reference >())
        {
            entry = resolved->as< instanciation_reference >();
        }
        else
        {
            initialization_reference empty_call;
            if (resolved->type_is< initialization_reference >())
            {
                empty_call = resolved->as< initialization_reference >();
            }
            else
            {
                empty_call.initializee = *resolved;
            }
            entry = co_await rpnx::querygraph::request< instanciation_query >(std::move(empty_call));
        }
        if (!entry.has_value())
        {
            throw semantic_compilation_error("The Quxlang Cortado backend entry is not callable as a concrete function");
        }
        if (!entry->params.positional.empty() || !entry->params.named.empty() || co_await rpnx::querygraph::request< functanoid_return_type_query >(*entry) != type_symbol(int_type{.bits = 32, .has_sign = true}))
        {
            throw semantic_compilation_error("The Quxlang Cortado backend executable entry must have signature FUNCTION(): I32");
        }

        type_symbol const entry_symbol = *entry;
        result.entry_procedure = entry_symbol;
        queued_routines.insert(entry_symbol);
        vmir2::functanoid_routine3 entry_routine = co_await rpnx::querygraph::request< vm_procedure3_query >(*entry);
        dependencies const& direct = co_await rpnx::querygraph::request< direct_dependencies_query >(direct_dependencies_input{.symbol = entry_symbol, .set = dependency_set::native});
        collect_dependencies(entry_symbol, entry_routine, direct);
        result.routines.emplace(entry_symbol, std::move(entry_routine));
    }
    else
    {
        if (!target.module_configurations.contains("RUNTIME"))
        {
            throw semantic_compilation_error("unit_test_suite output requires MODULE(RUNTIME)::UNIT_TEST_MAIN");
        }
        type_symbol const runtime_context = absolute_module_reference{.module_name = "RUNTIME"};
        type_symbol const unit_test_main_name = subsymbol{
            .of = runtime_context,
            .name = "UNIT_TEST_MAIN",
        };
        std::optional< type_symbol > const resolved_main = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = runtime_context,
            .type = unit_test_main_name,
        });
        if (!resolved_main.has_value())
        {
            throw semantic_compilation_error("Could not resolve MODULE(RUNTIME)::UNIT_TEST_MAIN");
        }
        initialization_reference main_initialization;
        if (resolved_main->type_is< initialization_reference >())
        {
            main_initialization = resolved_main->get_as< initialization_reference >();
        }
        else
        {
            main_initialization.initializee = *resolved_main;
        }
        std::optional< instanciation_reference > const unit_test_main = resolved_main->type_is< instanciation_reference >() ? std::optional< instanciation_reference >(resolved_main->get_as< instanciation_reference >()) : co_await rpnx::querygraph::request< instanciation_query >(std::move(main_initialization));
        if (!unit_test_main.has_value())
        {
            throw semantic_compilation_error("MODULE(RUNTIME)::UNIT_TEST_MAIN is not callable as a concrete function");
        }
        if (!unit_test_main->params.positional.empty() || !unit_test_main->params.named.empty() || co_await rpnx::querygraph::request< functanoid_return_type_query >(*unit_test_main) != type_symbol(int_type{.bits = 32, .has_sign = true}))
        {
            throw semantic_compilation_error("MODULE(RUNTIME)::UNIT_TEST_MAIN must have signature FUNCTION(): I32");
        }
        type_symbol const unit_test_main_symbol = *unit_test_main;
        result.entry_procedure = unit_test_main_symbol;
        queued_routines.insert(unit_test_main_symbol);
        vmir2::functanoid_routine3 unit_test_main_routine = co_await rpnx::querygraph::request< vm_procedure3_query >(*unit_test_main);
        dependencies const& unit_test_main_dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(direct_dependencies_input{
            .symbol = unit_test_main_symbol,
            .set = dependency_set::native,
        });
        collect_dependencies(unit_test_main_symbol, unit_test_main_routine, unit_test_main_dependencies);
        result.routines.emplace(unit_test_main_symbol, std::move(unit_test_main_routine));

        std::set< type_symbol > tests;
        for (std::string const& module_name : output_info.module_names)
        {
            try
            {
                type_symbol const module_context = absolute_module_reference{.module_name = module_name};
                std::set< type_symbol > const module_tests = co_await rpnx::querygraph::request< list_unit_tests_query >(module_context);
                tests.insert(module_tests.begin(), module_tests.end());
            }
            catch (std::invalid_argument const& error)
            {
                throw lowering_compilation_error("Quxlang's Cortado backend cannot discover layout-independent unit tests for module " + module_name + ": " + error.what());
            }
        }
        for (type_symbol const& test : tests)
        {
            try
            {
                vmir2::functanoid_routine3 routine = co_await rpnx::querygraph::request< unit_test_vmir_query >(test);
                dependencies const& direct = co_await rpnx::querygraph::request< direct_dependencies_query >(direct_dependencies_input{.symbol = test, .set = dependency_set::native});
                result.unit_tests.push_back(cortado_backend::unit_test_entry{
                    .name = to_string(test),
                    .procedure_symbol = test,
                });
                queued_routines.insert(test);
                collect_dependencies(test, routine, direct);
                result.routines.emplace(test, std::move(routine));
            }
            catch (compilation_error& error)
            {
                error.traceback.push_back(trace_frame{
                    .trace_context = "unit test " + to_string(test),
                    .location = std::nullopt,
                });
                throw;
            }
            catch (std::invalid_argument const& error)
            {
                throw lowering_compilation_error("Quxlang's Cortado backend cannot generate layout-independent VMIR for unit test " + to_string(test) + ": " + error.what());
            }
        }
    }

    while (true)
    {
        while (!pending_antestatal_globals.empty())
        {
            type_symbol global = std::move(pending_antestatal_globals.back());
            pending_antestatal_globals.pop_back();
            try
            {
                dependencies const& direct = co_await rpnx::querygraph::request< direct_dependencies_query >(direct_dependencies_input{
                    .symbol = global,
                    .set = dependency_set::native,
                });
                collect_dependency_references(direct);
            }
            catch (compilation_error& error)
            {
                error.traceback.push_back(trace_frame{
                    .trace_context = "reached antestatal global " + to_string(global),
                    .location = std::nullopt,
                });
                throw;
            }
        }

        while (!pending_routines.empty())
        {
            instanciation_reference functanoid = std::move(pending_routines.back());
            pending_routines.pop_back();
            type_symbol const symbol = functanoid;
            try
            {
                type_symbol const declaration_symbol = functanoid.temploid.templexoid;
                ast2_symboid const& declaration = co_await rpnx::querygraph::request< symboid_query >(declaration_symbol);
                if (declaration.type_is< ast2_asm_procedure_declaration >())
                {
                    throw lowering_compilation_error("Quxlang's Cortado backend does not support reached ASM_PROCEDURE: " + to_string(symbol));
                }
                if (declaration.type_is< ast2_extern_procedure >())
                {
                    ast2_extern_procedure const& external_declaration = declaration.get_as< ast2_extern_procedure >();
                    if (!valid_jvm_internal_name(external_declaration.external_scope_name))
                    {
                        throw semantic_compilation_error("JVM EXTERN_PROCEDURE owner must be a slash-separated internal class name: " + external_declaration.external_scope_name);
                    }
                    if (external_declaration.version.has_value() || external_declaration.is_optional)
                    {
                        throw semantic_compilation_error("JVM EXTERN_PROCEDURE does not support VERSION or OPTIONAL: " + to_string(symbol));
                    }
                    if (!external_declaration.callable.has_value())
                    {
                        throw semantic_compilation_error("Reached JVM EXTERN_PROCEDURE has no CALLABLE interface: " + to_string(symbol));
                    }

                    asm_procedure const resolved_procedure = co_await rpnx::querygraph::request< asm_procedure_from_symbol_query >(symbol);
                    if (!resolved_procedure.callable_interface.has_value())
                    {
                        throw semantic_compilation_error("Reached JVM EXTERN_PROCEDURE has no resolved CALLABLE interface: " + to_string(symbol));
                    }
                    asm_callable const& callable = *resolved_procedure.callable_interface;
                    if (!callable.clobber.empty() || callable.return_register_name.has_value())
                    {
                        throw semantic_compilation_error("JVM EXTERN_PROCEDURE CALLABLE interfaces cannot declare registers or clobbers: " + to_string(symbol));
                    }

                    cortado_backend::jvm_call_kind call_kind;
                    if (callable.calling_conv == "JVM_STATIC")
                    {
                        call_kind = cortado_backend::jvm_call_kind::static_method;
                    }
                    else if (callable.calling_conv == "JVM_VIRTUAL")
                    {
                        call_kind = cortado_backend::jvm_call_kind::virtual_method;
                    }
                    else if (callable.calling_conv == "JVM_INTERFACE")
                    {
                        call_kind = cortado_backend::jvm_call_kind::interface_method;
                    }
                    else if (callable.calling_conv == "JVM_CONSTRUCTOR")
                    {
                        call_kind = cortado_backend::jvm_call_kind::constructor;
                    }
                    else if (callable.calling_conv == "JVM_GETSTATIC")
                    {
                        call_kind = cortado_backend::jvm_call_kind::get_static;
                    }
                    else if (callable.calling_conv == "JVM_PUTSTATIC")
                    {
                        call_kind = cortado_backend::jvm_call_kind::put_static;
                    }
                    else if (callable.calling_conv == "JVM_GETFIELD")
                    {
                        call_kind = cortado_backend::jvm_call_kind::get_field;
                    }
                    else if (callable.calling_conv == "JVM_PUTFIELD")
                    {
                        call_kind = cortado_backend::jvm_call_kind::put_field;
                    }
                    else
                    {
                        throw semantic_compilation_error("Unsupported JVM EXTERN_PROCEDURE CALLCONV '" + callable.calling_conv + "' for " + to_string(symbol));
                    }

                    cortado_backend::resolved_jvm_external_callable resolved_call{
                        .owner_internal_name = external_declaration.external_scope_name,
                        .member_name = external_declaration.external_symbol_name,
                        .call_kind = call_kind,
                    };
                    for (asm_argument_binding const& argument : callable.args)
                    {
                        resolved_call.parameters.push_back(cortado_backend::jvm_external_parameter{
                            .api_name = argument.api_name,
                            .type = argument.type,
                        });
                        semantic_type_roots.insert(argument.type);
                        if (argument.type.type_is< ptrref_type >() && argument.type.get_as< ptrref_type >().ptr_class == pointer_class::gc)
                        {
                            ptrref_type const& pointer = argument.type.get_as< ptrref_type >();
                            ast2_symboid const external_type_declaration = co_await rpnx::querygraph::request< symboid_query >(pointer.target);
                            if (!external_type_declaration.type_is< ast2_extern_type >())
                            {
                                throw semantic_compilation_error("JVM GC pointer ABI parameters must point to EXTERN_TYPE: " + to_string(argument.type));
                            }
                            ast2_extern_type const& external_type = external_type_declaration.get_as< ast2_extern_type >();
                            if (external_type.source_name != "java.base" && external_type.source_name != "java.desktop")
                            {
                                throw semantic_compilation_error("Unresolved JVM dependency alias '" + external_type.source_name + "' for " + to_string(pointer.target));
                            }
                            if (!valid_jvm_internal_name(external_type.external_type_name))
                            {
                                throw semantic_compilation_error("EXTERN_TYPE JVM name must be slash-separated: " + external_type.external_type_name);
                            }
                            result.external_types.emplace(pointer.target, cortado_backend::jvm_external_type_info{
                                                                              .source_name = external_type.source_name,
                                                                              .internal_name = external_type.external_type_name,
                                                                          });
                        }
                    }
                    if (callable.return_type.has_value() && !callable.return_type->type_is< void_type >())
                    {
                        resolved_call.return_type = *callable.return_type;
                        semantic_type_roots.insert(*callable.return_type);
                        if (callable.return_type->type_is< ptrref_type >() && callable.return_type->get_as< ptrref_type >().ptr_class == pointer_class::gc)
                        {
                            ptrref_type const& pointer = callable.return_type->get_as< ptrref_type >();
                            ast2_symboid const external_type_declaration = co_await rpnx::querygraph::request< symboid_query >(pointer.target);
                            if (!external_type_declaration.type_is< ast2_extern_type >())
                            {
                                throw semantic_compilation_error("JVM GC pointer ABI returns must point to EXTERN_TYPE: " + to_string(*callable.return_type));
                            }
                            ast2_extern_type const& external_type = external_type_declaration.get_as< ast2_extern_type >();
                            if (external_type.source_name != "java.base" && external_type.source_name != "java.desktop")
                            {
                                throw semantic_compilation_error("Unresolved JVM dependency alias '" + external_type.source_name + "' for " + to_string(pointer.target));
                            }
                            if (!valid_jvm_internal_name(external_type.external_type_name))
                            {
                                throw semantic_compilation_error("EXTERN_TYPE JVM name must be slash-separated: " + external_type.external_type_name);
                            }
                            result.external_types.emplace(pointer.target, cortado_backend::jvm_external_type_info{
                                                                              .source_name = external_type.source_name,
                                                                              .internal_name = external_type.external_type_name,
                                                                          });
                        }
                    }

                    auto find_named_parameter = [&](std::string const& name) -> cortado_backend::jvm_external_parameter const*
                    {
                        for (cortado_backend::jvm_external_parameter const& parameter : resolved_call.parameters)
                        {
                            if (parameter.api_name == name)
                            {
                                return &parameter;
                            }
                        }
                        return nullptr;
                    };
                    cortado_backend::jvm_external_parameter const* const receiver = find_named_parameter("THIS");
                    cortado_backend::jvm_external_parameter const* const field_value = find_named_parameter("VALUE");
                    auto receiver_matches_owner = [&]() -> bool
                    {
                        if (receiver == nullptr || !receiver->type.type_is< ptrref_type >())
                        {
                            return false;
                        }
                        ptrref_type const& pointer = receiver->type.get_as< ptrref_type >();
                        if (pointer.ptr_class != pointer_class::gc)
                        {
                            return false;
                        }
                        std::map< type_symbol, cortado_backend::jvm_external_type_info >::const_iterator const external = result.external_types.find(pointer.target);
                        return external != result.external_types.end() && external->second.internal_name == resolved_call.owner_internal_name;
                    };

                    bool const is_instance_operation = call_kind == cortado_backend::jvm_call_kind::virtual_method || call_kind == cortado_backend::jvm_call_kind::interface_method || call_kind == cortado_backend::jvm_call_kind::get_field || call_kind == cortado_backend::jvm_call_kind::put_field;
                    if (is_instance_operation && !receiver_matches_owner())
                    {
                        throw semantic_compilation_error("JVM instance binding requires @THIS ~> EXTERN_TYPE matching owner " + resolved_call.owner_internal_name + ": " + to_string(symbol));
                    }
                    if (!is_instance_operation && receiver != nullptr)
                    {
                        throw semantic_compilation_error("JVM static and constructor bindings cannot declare @THIS: " + to_string(symbol));
                    }

                    std::size_t non_receiver_parameter_count = resolved_call.parameters.size() - (receiver == nullptr ? 0 : 1);
                    if (call_kind == cortado_backend::jvm_call_kind::constructor)
                    {
                        if (resolved_call.member_name != "<init>" || !resolved_call.return_type.has_value() || !resolved_call.return_type->type_is< ptrref_type >())
                        {
                            throw semantic_compilation_error("JVM_CONSTRUCTOR requires member <init> and a matching GC-pointer return: " + to_string(symbol));
                        }
                        ptrref_type const& return_pointer = resolved_call.return_type->get_as< ptrref_type >();
                        std::map< type_symbol, cortado_backend::jvm_external_type_info >::const_iterator const external = result.external_types.find(return_pointer.target);
                        if (return_pointer.ptr_class != pointer_class::gc || external == result.external_types.end() || external->second.internal_name != resolved_call.owner_internal_name)
                        {
                            throw semantic_compilation_error("JVM_CONSTRUCTOR return type must match owner " + resolved_call.owner_internal_name);
                        }
                    }
                    else if (resolved_call.member_name == "<init>")
                    {
                        throw semantic_compilation_error("Only JVM_CONSTRUCTOR may bind <init>: " + to_string(symbol));
                    }

                    if (call_kind == cortado_backend::jvm_call_kind::get_static)
                    {
                        if (non_receiver_parameter_count != 0 || !resolved_call.return_type.has_value())
                        {
                            throw semantic_compilation_error("JVM_GETSTATIC takes no arguments and must return a value: " + to_string(symbol));
                        }
                        resolved_call.descriptor = jvm_descriptor_for_type(*resolved_call.return_type);
                    }
                    else if (call_kind == cortado_backend::jvm_call_kind::put_static)
                    {
                        if (non_receiver_parameter_count != 1 || field_value == nullptr || resolved_call.return_type.has_value())
                        {
                            throw semantic_compilation_error("JVM_PUTSTATIC requires only @VALUE and no return: " + to_string(symbol));
                        }
                        resolved_call.descriptor = jvm_descriptor_for_type(field_value->type);
                    }
                    else if (call_kind == cortado_backend::jvm_call_kind::get_field)
                    {
                        if (non_receiver_parameter_count != 0 || !resolved_call.return_type.has_value())
                        {
                            throw semantic_compilation_error("JVM_GETFIELD requires only @THIS and must return a value: " + to_string(symbol));
                        }
                        resolved_call.descriptor = jvm_descriptor_for_type(*resolved_call.return_type);
                    }
                    else if (call_kind == cortado_backend::jvm_call_kind::put_field)
                    {
                        if (non_receiver_parameter_count != 1 || field_value == nullptr || resolved_call.return_type.has_value())
                        {
                            throw semantic_compilation_error("JVM_PUTFIELD requires @THIS and @VALUE with no return: " + to_string(symbol));
                        }
                        resolved_call.descriptor = jvm_descriptor_for_type(field_value->type);
                    }
                    else
                    {
                        resolved_call.descriptor = "(";
                        for (cortado_backend::jvm_external_parameter const& parameter : resolved_call.parameters)
                        {
                            if (parameter.api_name == "THIS")
                            {
                                continue;
                            }
                            resolved_call.descriptor += jvm_descriptor_for_type(parameter.type);
                        }
                        resolved_call.descriptor += ")";
                        resolved_call.descriptor += call_kind == cortado_backend::jvm_call_kind::constructor ? "V" : resolved_call.return_type.has_value() ? jvm_descriptor_for_type(*resolved_call.return_type) : "V";
                    }

                    result.external_callables.emplace(symbol, std::move(resolved_call));
                    continue;
                }

                vmir2::functanoid_routine3 routine = co_await rpnx::querygraph::request< vm_procedure3_query >(functanoid);
                dependencies const& direct = co_await rpnx::querygraph::request< direct_dependencies_query >(direct_dependencies_input{.symbol = symbol, .set = dependency_set::native});
                collect_dependencies(symbol, routine, direct);
                result.routines.emplace(symbol, std::move(routine));
            }
            catch (compilation_error& error)
            {
                error.traceback.push_back(trace_frame{
                    .trace_context = "reached routine " + to_string(symbol),
                    .location = std::nullopt,
                });
                throw;
            }
            catch (std::invalid_argument const& error)
            {
                throw lowering_compilation_error("Quxlang's Cortado backend cannot generate layout-independent VMIR for reached routine " + to_string(symbol) + ": " + error.what());
            }
        }

        std::optional< vmir_runtime_dependency > unresolved_runtime_dependency;
        for (vmir_runtime_dependency const dependency : result.runtime_requirements)
        {
            if (!result.resolved_runtime_procedures.contains(dependency))
            {
                unresolved_runtime_dependency = dependency;
                break;
            }
        }
        if (!unresolved_runtime_dependency.has_value())
        {
            if (pending_antestatal_globals.empty() && pending_routines.empty())
            {
                break;
            }
            continue;
        }
        if (!target.module_configurations.contains("RUNTIME"))
        {
            throw semantic_compilation_error("Quxlang's Cortado backend runtime lowering requires MODULE(RUNTIME)::" + runtime_procedure_name(*unresolved_runtime_dependency));
        }

        initialization_reference initialization = runtime_procedure_initialization(*unresolved_runtime_dependency);
        type_symbol const runtime_context = absolute_module_reference{.module_name = "RUNTIME"};
        std::optional< type_symbol > const resolved = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = runtime_context,
            .type = initialization,
        });
        if (!resolved.has_value())
        {
            throw semantic_compilation_error("Could not resolve MODULE(RUNTIME)::" + runtime_procedure_name(*unresolved_runtime_dependency));
        }
        std::optional< instanciation_reference > const instantiated = resolved->type_is< instanciation_reference >() ? std::optional< instanciation_reference >(resolved->get_as< instanciation_reference >()) : resolved->type_is< initialization_reference >() ? co_await rpnx::querygraph::request< instanciation_query >(resolved->get_as< initialization_reference >()) : std::nullopt;
        if (!instantiated.has_value())
        {
            throw semantic_compilation_error("MODULE(RUNTIME)::" + runtime_procedure_name(*unresolved_runtime_dependency) + " is not callable with the required runtime signature; lookup resolved to " + to_string(*resolved));
        }
        type_symbol const runtime_symbol = *instantiated;
        result.resolved_runtime_procedures.emplace(*unresolved_runtime_dependency, runtime_symbol);
        enqueue_routine(runtime_symbol);
    }

    for (type_symbol const& global : global_roots)
    {
        try
        {
            if (co_await rpnx::querygraph::request< global_is_per_thread_query >(global))
            {
                throw rpnx::unimplemented();
            }
            type_symbol const global_type = co_await rpnx::querygraph::request< variable_type_query >(global);
            result.global_types.emplace(global, global_type);
            semantic_type_roots.insert(global_type);
            if (co_await rpnx::querygraph::request< global_is_antestatal_static_query >(global))
            {
                result.global_values.emplace(global, co_await rpnx::querygraph::request< antestatal_static_value_query >(global));
            }
        }
        catch (compilation_error& error)
        {
            error.traceback.push_back(trace_frame{
                .trace_context = "reached global " + to_string(global),
                .location = std::nullopt,
            });
            throw;
        }
        catch (std::invalid_argument const& error)
        {
            throw lowering_compilation_error("Quxlang's Cortado backend cannot aggregate layout-independent global " + to_string(global) + ": " + error.what());
        }
    }

    std::set< type_symbol > seen_types;
    std::vector< type_symbol > pending_types;
    auto enqueue_type = [&](type_symbol const& type) -> void
    {
        if (seen_types.insert(type).second)
        {
            pending_types.push_back(type);
        }
    };
    for (type_symbol const& type : semantic_type_roots)
    {
        enqueue_type(type);
    }

    while (!pending_types.empty())
    {
        type_symbol type = std::move(pending_types.back());
        pending_types.pop_back();
        if (type.type_is< nvalue_slot >())
        {
            enqueue_type(type.as< nvalue_slot >().target);
            continue;
        }
        if (type.type_is< dvalue_slot >())
        {
            enqueue_type(type.as< dvalue_slot >().target);
            continue;
        }
        if (type.type_is< ptrref_type >())
        {
            enqueue_type(type.as< ptrref_type >().target);
            continue;
        }
        if (type.type_is< storage >())
        {
            result.storage_definitions.insert(type);
            for (type_symbol const& alternative : type.as< storage >().storable_types)
            {
                enqueue_type(alternative);
            }
            continue;
        }
        if (type.type_is< array_type >())
        {
            enqueue_type(type.as< array_type >().element_type);
            continue;
        }

        try
        {
            class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(type);
            if (kind == class_kind::struct_)
            {
                std::vector< struct_field > const fields = co_await rpnx::querygraph::request< struct_field_list_query >(type);
                result.struct_definitions.emplace(type, fields);
                for (struct_field const& field : fields)
                {
                    enqueue_type(field.type);
                }
            }
            else if (kind == class_kind::enum_)
            {
                result.enum_definitions.emplace(type, co_await rpnx::querygraph::request< enum_info_query >(type));
            }
            else if (kind == class_kind::flagset)
            {
                result.flagset_definitions.emplace(type, co_await rpnx::querygraph::request< flagset_info_query >(type));
            }
            else if (kind == class_kind::union_)
            {
                union_info const info = co_await rpnx::querygraph::request< union_info_query >(type);
                result.union_definitions.emplace(type, info);
                for (union_option_info const& option : info.options)
                {
                    enqueue_type(option.type);
                }
            }
            else if (kind == class_kind::variant)
            {
                variant_info const info = co_await rpnx::querygraph::request< variant_info_query >(type);
                result.variant_definitions.emplace(type, info);
                for (type_symbol const& alternative : info.alternatives)
                {
                    enqueue_type(alternative);
                }
            }
            else if (kind == class_kind::external)
            {
                ast2_symboid const declaration = co_await rpnx::querygraph::request< symboid_query >(type);
                if (!declaration.type_is< ast2_extern_type >())
                {
                    throw compiler_bug("External class kind did not resolve to EXTERN_TYPE: " + to_string(type));
                }
                ast2_extern_type const& external = declaration.get_as< ast2_extern_type >();
                if (external.source_name != "java.base" && external.source_name != "java.desktop")
                {
                    throw semantic_compilation_error("Unresolved JVM dependency alias '" + external.source_name + "' for " + to_string(type));
                }
                if (!valid_jvm_internal_name(external.external_type_name))
                {
                    throw semantic_compilation_error("EXTERN_TYPE JVM name must be slash-separated: " + external.external_type_name);
                }
                result.external_types.emplace(type, cortado_backend::jvm_external_type_info{
                                                        .source_name = external.source_name,
                                                        .internal_name = external.external_type_name,
                                                    });
            }
        }
        catch (std::invalid_argument const& error)
        {
            throw lowering_compilation_error("Quxlang's Cortado backend cannot aggregate layout-independent semantic type " + to_string(type) + ": " + error.what());
        }
    }

    co_return result;
}
