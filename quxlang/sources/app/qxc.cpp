// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include "quxlang/blake2b.hpp"
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/compiler_querygraph.hpp"
#include "quxlang/llvm-backend.hpp"
#include "quxlang/manipulators/mangler.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/queries/asm_procedure_from_symbol.hpp"
#include "quxlang/queries/antestatal_static_value.hpp"
#include "quxlang/queries/struct_layout.hpp"
#include "quxlang/queries/enum_info.hpp"
#include "quxlang/queries/flagset_info.hpp"
#include "quxlang/queries/fusion_layout.hpp"
#include "quxlang/queries/functanoid_return_type.hpp"
#include "quxlang/queries/global_init_type.hpp"
#include "quxlang/queries/global_is_antestatal_static.hpp"
#include "quxlang/queries/interface_slot_list.hpp"
#include "quxlang/queries/instanciation.hpp"
#include "quxlang/queries/list_static_tests.hpp"
#include "quxlang/queries/list_unit_tests.hpp"
#include "quxlang/queries/lookup.hpp"
#include "quxlang/queries/llvm_compiled_output.hpp"
#include "quxlang/queries/output_binaries_information.hpp"
#include "quxlang/queries/output_llvm_input.hpp"
#include "quxlang/queries/output_binary_artifacts.hpp"
#include "quxlang/queries/output_llvm_backend_options.hpp"
#include "quxlang/queries/output_optimized_llvm.hpp"
#include "quxlang/queries/output_unoptimized_llvm.hpp"
#include "quxlang/queries/procedure_linksymbol.hpp"
#include "quxlang/queries/static_test_vmir.hpp"
#include "quxlang/queries/symboid.hpp"
#include "quxlang/queries/symbol_type.hpp"
#include "quxlang/queries/class_type.hpp"
#include "quxlang/queries/temploid_formal_ensig.hpp"
#include "quxlang/queries/class_placement_info.hpp"
#include "quxlang/queries/unit_test_vmir.hpp"
#include "quxlang/queries/union_info.hpp"
#include "quxlang/queries/variable_type.hpp"
#include "quxlang/queries/variant_info.hpp"
#include "quxlang/queries/vm_procedure3.hpp"
#include "quxlang/queries/vmir_dependencies.hpp"
#include "quxlang/source_loader.hpp"
#include "quxlang/vmir2/assembler.hpp"
#include "quxlang/vmir2/routine_requirements.hpp"
#include "quxlang/vmir2/source_index.hpp"
#include "qxc_llvm_inlining.hpp"
#include "qxc_output_paths.hpp"

#include <rpnx/serialization4.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    /** Returns a deterministic BLAKE2b-512 digest of the loaded source bundle. */
    auto source_bundle_hash(quxlang::source_bundle const& bundle) -> std::string
    {
        std::vector< std::byte > serialized;
        rpnx::serial4::serialize_iter(bundle, std::back_inserter(serialized));
        return quxlang::blake2b::hex(serialized);
    }

    /**
     * Builds the same source file index as source_file_index_query, without
     * requiring a querygraph request during qxc diagnostic setup.
     */
    auto make_source_file_index(quxlang::source_bundle const& bundle) -> quxlang::source_file_index
    {
        quxlang::source_file_index result;
        std::uint64_t next_id = 0;

        for (auto const& [source_module, module] : bundle.module_sources)
        {
            for (auto const& [relative_path, file] : module.files)
            {
                (void)file;
                quxlang::source_file_name name{.source_module = source_module, .relative_path = relative_path};
                result.id_to_file.emplace(next_id, name);
                result.file_to_id.emplace(std::move(name), next_id);
                ++next_id;
            }
        }

        return result;
    }

    /**
     * Parses the optional command-line target filter list.
     */
    auto parse_target_filters(int arg_count, char** arg_values) -> std::optional< std::set< std::string > >
    {
        if (arg_count <= 3)
        {
            return std::nullopt;
        }

        std::set< std::string > targets;
        for (int i = 3; i < arg_count; i++)
        {
            if (std::string_view(arg_values[i]) == "--debug-compile-output")
            {
                continue;
            }

            std::stringstream target_stream(arg_values[i]);
            std::string target_name;
            while (std::getline(target_stream, target_name, ','))
            {
                if (!target_name.empty())
                {
                    targets.insert(target_name);
                }
            }
        }

        return targets;
    }

    /**
     * Returns true when qxc should emit detailed VMIR/LLVM/object diagnostic artifacts.
     */
    auto parse_debug_compile_output(int arg_count, char** arg_values) -> bool
    {
        for (int i = 3; i < arg_count; i++)
        {
            if (std::string_view(arg_values[i]) == "--debug-compile-output")
            {
                return true;
            }
        }

        return false;
    }

    /**
     * Converts configured LLVM backend options into the qxc LLVM backend optimization level.
     */
    auto configured_llvm_optimization_level(quxlang::backend_llvm_options const& options)
        -> quxlang::llvm_backend::optimization_level
    {
        if (options.mode == quxlang::backend_llvm_mode::debug)
        {
            return quxlang::llvm_backend::optimization_level::debug;
        }
        return quxlang::llvm_backend::optimization_level::release;
    }

    /**
     * Converts a referenced routine symbol into the concrete functanoid qxc can emit.
     */
    auto concrete_functanoid_from_symbol(quxlang::compiler_querygraph& graph, quxlang::type_symbol const& symbol) -> std::optional< quxlang::instanciation_reference >
    {
        return rpnx::apply_visitor< std::optional< quxlang::instanciation_reference > >(
            symbol,
            [&](auto const& selected_symbol) -> std::optional< quxlang::instanciation_reference >
            {
                using selected_symbol_type = std::decay_t< decltype(selected_symbol) >;

                if constexpr (std::is_same_v< selected_symbol_type, quxlang::instanciation_reference >)
                {
                    return selected_symbol;
                }
                else if constexpr (std::is_same_v< selected_symbol_type, quxlang::temploid_reference >)
                {
                    std::optional< quxlang::temploid_ensig > formal_ensig = graph.make_request< quxlang::temploid_formal_ensig_query >(selected_symbol);
                    if (!formal_ensig.has_value())
                    {
                        throw quxlang::semantic_compilation_error("Cannot resolve selected overload for procedure pointer target: " + quxlang::to_string(symbol));
                    }

                    if (overload_has_unspecialized_parameters(*formal_ensig))
                    {
                        throw quxlang::semantic_compilation_error("Cannot emit uninstantiated procedure pointer target: " + quxlang::to_string(symbol));
                    }

                    return quxlang::instanciation_reference{
                        .temploid = selected_symbol,
                        .params = instantiate_declared_overload(*formal_ensig),
                    };
                }
                else
                {
                    return std::nullopt;
                }
            });
    }

    using functanoid_reference_locations = std::map< quxlang::type_symbol, std::optional< quxlang::source_location > >;
    using runtime_procedure_reference_locations =
        std::map< quxlang::llvm_backend::runtime_procedure_reference, std::optional< quxlang::source_location > >;

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

    void validate_executable_entry_signature(quxlang::compiler_querygraph& graph, quxlang::instanciation_reference const& entry_functanoid)
    {
        if (!entry_functanoid.params.positional.empty() || !entry_functanoid.params.named.empty())
        {
            throw quxlang::semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + quxlang::to_string(entry_functanoid));
        }

        quxlang::type_symbol const return_type = graph.make_request< quxlang::functanoid_return_type_query >(entry_functanoid);
        if (return_type != quxlang::type_symbol(quxlang::int_type{.bits = 32, .has_sign = true}))
        {
            throw quxlang::semantic_compilation_error("Executable entry functanoid must have signature PROCEDURE(: I32): " + quxlang::to_string(entry_functanoid));
        }
    }

    /**
     * Records a concrete functanoid dependency and the source location that referenced it.
     */
    void record_referenced_functanoid(quxlang::compiler_querygraph& graph, functanoid_reference_locations& result, quxlang::type_symbol const& symbol, std::optional< quxlang::source_location > location)
    {
        std::optional< quxlang::instanciation_reference > concrete = concrete_functanoid_from_symbol(graph, symbol);
        if (!concrete.has_value() && symbol.type_is< quxlang::initialization_reference >())
        {
            concrete = graph.make_request< quxlang::instanciation_query >(symbol.get_as< quxlang::initialization_reference >());
        }
        if (!concrete.has_value())
        {
            throw quxlang::compiler_bug("VMIR2 routine references a non-functanoid symbol: " + quxlang::to_string(symbol));
        }

        quxlang::type_symbol concrete_symbol = *concrete;
        std::pair< functanoid_reference_locations::iterator, bool > insertion = result.emplace(concrete_symbol, location);
        if (!insertion.second && !insertion.first->second.has_value() && location.has_value())
        {
            insertion.first->second = location;
        }
    }

    /**
     * Records an abstract runtime procedure dependency and the source location that required it.
     */
    void record_referenced_runtime_procedure(
        runtime_procedure_reference_locations& result,
        quxlang::llvm_backend::runtime_procedure_reference const& reference,
        std::optional< quxlang::source_location > location)
    {
        std::pair< runtime_procedure_reference_locations::iterator, bool > insertion = result.emplace(reference, location);
        if (!insertion.second && !insertion.first->second.has_value() && location.has_value())
        {
            insertion.first->second = location;
        }
    }

    /**
     * Finds direct functanoid dependencies of a VMIR2 routine and keeps source locations for invoke-like references.
     */
    auto directly_referenced_functanoid_locations(quxlang::compiler_querygraph& graph, quxlang::type_symbol const& symbol) -> functanoid_reference_locations
    {
        functanoid_reference_locations result;
        quxlang::dependencies const& dependencies = graph.make_request< quxlang::direct_dependencies_query >(
            quxlang::direct_dependencies_input{.symbol = symbol, .set = quxlang::dependency_set::native});
        for (auto const& [symbol, location] : dependencies.functanoids)
        {
            record_referenced_functanoid(graph, result, symbol, location);
        }

        return result;
    }

    /**
     * Finds abstract runtime procedures required by native lowering of one VMIR2 routine.
     */
    auto directly_referenced_runtime_procedure_locations(quxlang::compiler_querygraph& graph, quxlang::type_symbol const& symbol) -> runtime_procedure_reference_locations
    {
        runtime_procedure_reference_locations result;
        quxlang::dependencies const& dependencies = graph.make_request< quxlang::direct_dependencies_query >(
            quxlang::direct_dependencies_input{.symbol = symbol, .set = quxlang::dependency_set::native});
        for (quxlang::vmir_runtime_dependency const dependency : dependencies.runtime_dependencies)
        {
            quxlang::llvm_backend::runtime_procedure procedure;
            switch (dependency)
            {
            case quxlang::vmir_runtime_dependency::assert_fail: procedure = quxlang::llvm_backend::runtime_procedure::assert_fail; break;
            case quxlang::vmir_runtime_dependency::panic: procedure = quxlang::llvm_backend::runtime_procedure::panic; break;
            case quxlang::vmir_runtime_dependency::initguard_complete: procedure = quxlang::llvm_backend::runtime_procedure::initguard_complete; break;
            case quxlang::vmir_runtime_dependency::initguard_abort: procedure = quxlang::llvm_backend::runtime_procedure::initguard_abort; break;
            case quxlang::vmir_runtime_dependency::initguard_try_acquire: procedure = quxlang::llvm_backend::runtime_procedure::initguard_try_acquire; break;
            }
            record_referenced_runtime_procedure(result, quxlang::llvm_backend::runtime_procedure_reference{.procedure = procedure}, std::nullopt);
        }

        return result;
    }

    /**
     * Builds the traceback carried from a caller routine to one directly referenced dependency.
     */
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

    /**
     * Resolves the configured output entry point to a concrete functanoid.
     */
    auto resolve_entry_functanoid(quxlang::compiler_querygraph& graph, std::string const& module_name, quxlang::type_symbol const& main_functanoid)
        -> quxlang::instanciation_reference
    {
        quxlang::type_symbol const module_context = quxlang::absolute_module_reference{.module_name = module_name};
        quxlang::type_symbol contextual_entry = quxlang::with_context(main_functanoid, module_context);

        auto resolved_entry = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
            .context = module_context,
            .type = contextual_entry,
        });

        if (!resolved_entry.has_value())
        {
            throw quxlang::semantic_compilation_error("Could not resolve main functanoid '" + quxlang::to_string(main_functanoid) + "' in module '" + module_name + "'");
        }

        if (auto concrete = concrete_functanoid_from_symbol(graph, *resolved_entry); concrete.has_value())
        {
            return *concrete;
        }

        quxlang::initialization_reference empty_call;
        if (resolved_entry->type_is< quxlang::initialization_reference >())
        {
            empty_call = resolved_entry->as< quxlang::initialization_reference >();
        }
        else
        {
            empty_call.initializee = *resolved_entry;
        }

        auto instanciation = graph.make_request< quxlang::instanciation_query >(std::move(empty_call));
        if (!instanciation.has_value())
        {
            throw quxlang::semantic_compilation_error("Main functanoid '" + quxlang::to_string(main_functanoid) + "' in module '" + module_name + "' is not callable as a concrete function");
        }

        return *instanciation;
    }

    /**
     * Returns the runtime entrypoint declaration name for one output kind.
     */
    auto runtime_start_name(quxlang::output_kind kind) -> std::string
    {
        if (kind == quxlang::output_kind::unit_test_suite)
        {
            return "UNIT_TESTING_PROGRAM_START";
        }
        return "PROGRAM_START";
    }

    /**
     * Finds the optional runtime-provided Linux process entrypoint declaration for one target.
     */
    auto try_resolve_runtime_entrypoint(
        quxlang::compiler_querygraph& graph,
        quxlang::target_configuration const& target_config,
        std::string const& entrypoint_name) -> std::optional< quxlang::type_symbol >
    {
        if (!target_config.module_configurations.contains("RUNTIME"))
        {
            return std::nullopt;
        }

        quxlang::type_symbol runtime_start = quxlang::subsymbol{
            .of = quxlang::absolute_module_reference{.module_name = "RUNTIME"},
            .name = entrypoint_name,
        };
        quxlang::ast2_symboid const symboid = graph.make_request< quxlang::symboid_query >(runtime_start);
        if (symboid.type_is< std::monostate >())
        {
            return std::nullopt;
        }
        if (!symboid.type_is< quxlang::ast2_asm_procedure_declaration >())
        {
            throw quxlang::semantic_compilation_error("RUNTIME::" + entrypoint_name + " must be an ASM_PROCEDURE");
        }

        return runtime_start;
    }

    /**
     * Resolves one abstract runtime procedure role to the concrete functanoid initialized for a target.
     */
    auto resolve_runtime_procedure_symbol(
        quxlang::compiler_querygraph& graph,
        quxlang::target_configuration const& target_config,
        quxlang::llvm_backend::runtime_procedure_reference const& reference) -> quxlang::type_symbol
    {
        if (!target_config.module_configurations.contains("RUNTIME"))
        {
            if (reference.procedure == quxlang::llvm_backend::runtime_procedure::panic)
            {
                throw quxlang::semantic_compilation_error("Native PANIC lowering requires MODULE(RUNTIME)::PANIC");
            }
            throw quxlang::semantic_compilation_error("Native ASSERT lowering requires MODULE(RUNTIME)::ASSERT_FAIL");
        }

        quxlang::initialization_reference runtime_init = quxlang::llvm_backend::runtime_procedure_initialization(reference.procedure);
        runtime_init.context = quxlang::absolute_module_reference{.module_name = "RUNTIME"};
        std::optional< quxlang::type_symbol > runtime_lookup = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
            .context = quxlang::absolute_module_reference{.module_name = "RUNTIME"},
            .type = std::move(runtime_init),
        });

        if (!runtime_lookup.has_value())
        {
            if (reference.procedure == quxlang::llvm_backend::runtime_procedure::panic)
            {
                throw quxlang::semantic_compilation_error("Native PANIC lowering requires MODULE(RUNTIME)::PANIC");
            }
            throw quxlang::semantic_compilation_error("Could not resolve runtime procedure: " + quxlang::to_string(quxlang::llvm_backend::runtime_procedure_initializee(reference.procedure)));
        }
        if (runtime_lookup->type_is< quxlang::instanciation_reference >())
        {
            return *runtime_lookup;
        }
        if (!runtime_lookup->type_is< quxlang::initialization_reference >())
        {
            if (reference.procedure == quxlang::llvm_backend::runtime_procedure::panic)
            {
                throw quxlang::semantic_compilation_error("Native PANIC lowering requires MODULE(RUNTIME)::PANIC");
            }
            throw quxlang::semantic_compilation_error("Runtime procedure did not resolve to an initialization reference: " + quxlang::to_string(quxlang::llvm_backend::runtime_procedure_initializee(reference.procedure)));
        }

        std::optional< quxlang::instanciation_reference > runtime_instanciation =
            graph.make_request< quxlang::instanciation_query >(runtime_lookup->as< quxlang::initialization_reference >());
        if (!runtime_instanciation.has_value())
        {
            if (reference.procedure == quxlang::llvm_backend::runtime_procedure::panic)
            {
                throw quxlang::semantic_compilation_error("Native PANIC lowering requires MODULE(RUNTIME)::PANIC");
            }
            throw quxlang::semantic_compilation_error("Could not initialize runtime procedure: " + quxlang::to_string(quxlang::llvm_backend::runtime_procedure_initializee(reference.procedure)));
        }

        return *runtime_instanciation;
    }

    /**
     * Writes one VMIR2 routine text file for qxc output.
     */
    auto write_vmir2_text_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_vmir2_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(ir_path.parent_path());

        std::ofstream outfile(ir_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open VMIR2 output file: " + ir_path.string());
        }

        outfile.write(ir_text.data(), static_cast< std::streamsize >(ir_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write VMIR2 output file: " + ir_path.string());
        }

        return ir_path;
    }

    /**
     * Writes one textual LLVM IR file for qxc output.
     */
    auto write_input_llvm_text_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_input_llvm_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const symbol_comment = "; Quxlang symbol: " + quxlang::to_string(functanoid_symbol) + "\n\n";

        std::ofstream outfile(ir_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open LLVM output file: " + ir_path.string());
        }

        outfile.write(symbol_comment.data(), static_cast< std::streamsize >(symbol_comment.size()));
        outfile.write(ir_text.data(), static_cast< std::streamsize >(ir_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write LLVM output file: " + ir_path.string());
        }

        return ir_path;
    }

    /**
     * Writes one final textual LLVM IR file for qxc output.
     */
    auto write_final_llvm_text_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_final_llvm_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const symbol_comment = "; Quxlang symbol: " + quxlang::to_string(functanoid_symbol) + "\n\n";

        std::ofstream outfile(ir_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open final LLVM output file: " + ir_path.string());
        }

        outfile.write(symbol_comment.data(), static_cast< std::streamsize >(symbol_comment.size()));
        outfile.write(ir_text.data(), static_cast< std::streamsize >(ir_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write final LLVM output file: " + ir_path.string());
        }

        return ir_path;
    }

    /**
     * Writes one input LLVM object file for qxc output.
     */
    auto write_input_object_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::vector< std::byte > const& object_bytes) -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_input_object_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(object_path.parent_path());

        std::ofstream outfile(object_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open LLVM object output file: " + object_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(object_bytes.data()), static_cast< std::streamsize >(object_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write LLVM object output file: " + object_path.string());
        }

        return object_path;
    }

    /**
     * Writes one final LLVM object file for qxc output.
     */
    auto write_final_object_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::vector< std::byte > const& object_bytes)
        -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_final_object_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(object_path.parent_path());

        std::ofstream outfile(object_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open final LLVM object output file: " + object_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(object_bytes.data()), static_cast< std::streamsize >(object_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write final LLVM object output file: " + object_path.string());
        }

        return object_path;
    }

    /**
     * Writes one assembly text file for a standalone asm procedure.
     */
    auto write_asm_source_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::string const& assembly_text) -> std::filesystem::path
    {
        std::filesystem::path const asm_path = quxlang::qxc_detail::make_asm_source_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(asm_path.parent_path());
        std::string const symbol_comment = "# Quxlang symbol: " + quxlang::to_string(functanoid_symbol) + "\n\n";

        std::ofstream outfile(asm_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open asm source output file: " + asm_path.string());
        }

        outfile.write(symbol_comment.data(), static_cast< std::streamsize >(symbol_comment.size()));
        outfile.write(assembly_text.data(), static_cast< std::streamsize >(assembly_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write asm source output file: " + asm_path.string());
        }

        return asm_path;
    }

    /**
     * Writes one standalone asm object file for qxc output.
     */
    auto write_asm_object_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::vector< std::byte > const& object_bytes)
        -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_asm_object_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(object_path.parent_path());

        std::ofstream outfile(object_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open asm object output file: " + object_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(object_bytes.data()), static_cast< std::streamsize >(object_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write asm object output file: " + object_path.string());
        }

        return object_path;
    }

    /**
     * Writes one aggregated input output-module LLVM IR file for qxc output.
     */
    auto write_output_module_input_llvm_text_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        quxlang::type_symbol const& entry_symbol,
        std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_output_module_input_llvm_output_path(build_dir, output_name);
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const header_comment =
            "; Quxlang output module: " + output_name + "\n"
            "; Quxlang entry: " + quxlang::to_string(entry_symbol) + "\n\n";

        std::ofstream outfile(ir_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open output-module LLVM file: " + ir_path.string());
        }

        outfile.write(header_comment.data(), static_cast< std::streamsize >(header_comment.size()));
        outfile.write(ir_text.data(), static_cast< std::streamsize >(ir_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write output-module LLVM file: " + ir_path.string());
        }

        return ir_path;
    }

    /**
     * Writes one aggregated final output-module LLVM IR file for qxc output.
     */
    auto write_output_module_final_llvm_text_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        quxlang::type_symbol const& entry_symbol,
        std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_output_module_final_llvm_output_path(build_dir, output_name);
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const header_comment =
            "; Quxlang output module: " + output_name + "\n"
            "; Quxlang entry: " + quxlang::to_string(entry_symbol) + "\n\n";

        std::ofstream outfile(ir_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open final output-module LLVM file: " + ir_path.string());
        }

        outfile.write(header_comment.data(), static_cast< std::streamsize >(header_comment.size()));
        outfile.write(ir_text.data(), static_cast< std::streamsize >(ir_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write final output-module LLVM file: " + ir_path.string());
        }

        return ir_path;
    }

    /**
     * Writes one aggregated input output-module LLVM object file for qxc output.
     */
    auto write_output_module_input_object_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        std::vector< std::byte > const& object_bytes) -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_output_module_input_object_output_path(build_dir, output_name);
        std::filesystem::create_directories(object_path.parent_path());

        std::ofstream outfile(object_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open output-module LLVM object file: " + object_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(object_bytes.data()), static_cast< std::streamsize >(object_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write output-module LLVM object file: " + object_path.string());
        }

        return object_path;
    }

    /**
     * Writes one final qxc output artifact using the configured output name directly.
     */
    auto write_final_output_file(
        std::filesystem::path const& output_dir,
        std::string const& output_name,
        std::vector< std::byte > const& file_bytes) -> std::filesystem::path
    {
        std::filesystem::path const executable_path = output_dir / output_name;
        std::filesystem::create_directories(executable_path.parent_path());

        std::ofstream outfile(executable_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open output executable file: " + executable_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(file_bytes.data()), static_cast< std::streamsize >(file_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write output executable file: " + executable_path.string());
        }

        std::filesystem::permissions(
            executable_path,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec | std::filesystem::perms::group_read |
                std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
            std::filesystem::perm_options::replace);

        return executable_path;
    }

    /**
     * Writes one aggregated final output-module LLVM object file for qxc output.
     */
    auto write_output_module_final_object_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        std::vector< std::byte > const& object_bytes) -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_output_module_final_object_output_path(build_dir, output_name);
        std::filesystem::create_directories(object_path.parent_path());

        std::ofstream outfile(object_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open final output-module LLVM object file: " + object_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(object_bytes.data()), static_cast< std::streamsize >(object_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write final output-module LLVM object file: " + object_path.string());
        }

        return object_path;
    }

    /**
     * Shared LLVM packet support data reused when emitting one routine tree as textual LLVM IR.
     */
    struct llvm_packet_support_data
    {
        std::map< quxlang::type_symbol, std::string > procedure_linksymbols;
        std::map< quxlang::type_symbol, quxlang::type_symbol > object_reference_types;
        std::map< quxlang::type_symbol, quxlang::antestatal_value > antestatal_constants;
        std::map< quxlang::type_symbol, quxlang::initialization_type > global_init_types;
        std::map< quxlang::type_symbol, std::vector< quxlang::interface_slot_key > > interface_slots;
        std::map< quxlang::type_symbol, quxlang::enum_info > enum_infos;
        std::map< quxlang::type_symbol, quxlang::flagset_info > flagset_infos;
        std::map< quxlang::type_symbol, quxlang::struct_layout > struct_layouts;
        std::map< quxlang::type_symbol, quxlang::union_info > union_infos;
        std::map< quxlang::type_symbol, quxlang::variant_info > variant_infos;
        std::map< quxlang::type_symbol, quxlang::fusion_layout > fusion_layouts;
        std::map< quxlang::type_symbol, quxlang::class_placement_info > type_placements;
    };

    /**
     * One fully discovered routine tree rooted at a single emitted functanoid.
     */
    struct collected_routine_tree
    {
        std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 > routines;
        std::map< quxlang::type_symbol, quxlang::asm_callable > asm_callable_interfaces;
        std::map< quxlang::type_symbol, quxlang::asm_procedure > asm_routines;
        std::map< quxlang::llvm_backend::runtime_procedure_reference, quxlang::type_symbol > runtime_procedures;
        std::set< quxlang::type_symbol > object_references;
        quxlang::qxc_detail::llvm_inlining_dependency_graph dependency_graph;
        llvm_packet_support_data support;
    };

    /**
     * Collects the type and readonly-global inputs needed to lower one routine tree to LLVM IR.
     */
    auto build_llvm_packet_support_data(
        quxlang::compiler_querygraph& graph,
        quxlang::machine_target_info const& machine,
        std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 > const& routines,
        std::set< quxlang::type_symbol > const& object_references) -> llvm_packet_support_data
    {
        llvm_packet_support_data result;

        std::set< quxlang::type_symbol > seen_types;
        std::vector< quxlang::type_symbol > pending_types;
        std::set< quxlang::type_symbol > seen_antestatal_globals;
        std::vector< quxlang::type_symbol > pending_antestatal_globals;

        auto enqueue_type = [&](quxlang::type_symbol const& type) -> void
        {
            if (seen_types.insert(type).second)
            {
                pending_types.push_back(type);
            }
        };

        auto enqueue_antestatal_global = [&](quxlang::type_symbol const& symbol) -> void
        {
            if (seen_antestatal_globals.insert(symbol).second)
            {
                pending_antestatal_globals.push_back(symbol);
            }
        };

        for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& routine_entry : routines)
        {
            quxlang::vmir2::functanoid_routine3 const& routine = routine_entry.second;
            quxlang::dependencies const& dependencies = graph.make_request< quxlang::direct_dependencies_query >(
                quxlang::direct_dependencies_input{.symbol = routine_entry.first, .set = quxlang::dependency_set::native});
            for (quxlang::type_symbol const& placement_root : dependencies.type_placements)
            {
                enqueue_type(placement_root);
            }
            for (quxlang::type_symbol const& struct_root : dependencies.struct_layouts)
            {
                enqueue_type(struct_root);
            }
            for (quxlang::type_symbol const& fusion_root : dependencies.fusion_layouts)
            {
                enqueue_type(fusion_root);
            }

            for (quxlang::type_symbol const& antestatal_root : dependencies.antestatal_globals)
            {
                enqueue_antestatal_global(antestatal_root);
            }

            for (quxlang::type_symbol const& global_root : dependencies.global_roots)
            {
                result.global_init_types[global_root] = graph.make_request< quxlang::global_init_type_query >(global_root);
            }

            for (quxlang::static_snapshot_ref const& snapshot : dependencies.static_snapshots)
            {
                quxlang::vmir2::localdata_entry const& snapshot_entry = routine.static_snapshots.at(snapshot);
                quxlang::type_symbol const snapshot_symbol = quxlang::type_symbol(snapshot);
                result.antestatal_constants[snapshot_symbol] = snapshot_entry.value;
                result.object_reference_types[snapshot_symbol] = snapshot_entry.type;
                enqueue_type(snapshot_entry.type);
            }
        }

        for (quxlang::type_symbol const& object_reference : object_references)
        {
            if (is_main_function_object_symbol(object_reference))
            {
                quxlang::type_symbol const object_type = main_function_object_type();
                result.object_reference_types.emplace(object_reference, object_type);
                enqueue_type(object_type);
                continue;
            }
            if (std::optional< quxlang::type_symbol > const unit_test_object_type = quxlang::llvm_backend::unit_test_object_type(object_reference); unit_test_object_type.has_value())
            {
                result.object_reference_types.emplace(object_reference, *unit_test_object_type);
                enqueue_type(*unit_test_object_type);
                continue;
            }

            if (graph.make_request< quxlang::symbol_type_query >(object_reference) != quxlang::symbol_kind::global_variable)
            {
                throw quxlang::semantic_compilation_error("OBJECT_REF target is not a global object: " + quxlang::to_string(object_reference));
            }

            quxlang::type_symbol const object_type = graph.make_request< quxlang::variable_type_query >(object_reference);
            result.object_reference_types.emplace(object_reference, object_type);
            result.global_init_types[object_reference] = graph.make_request< quxlang::global_init_type_query >(object_reference);
            enqueue_type(object_type);
            if (graph.make_request< quxlang::global_is_antestatal_static_query >(object_reference))
            {
                enqueue_antestatal_global(object_reference);
            }
        }

        while (!pending_antestatal_globals.empty())
        {
            quxlang::type_symbol symbol = std::move(pending_antestatal_globals.back());
            pending_antestatal_globals.pop_back();

            if (!graph.make_request< quxlang::global_is_antestatal_static_query >(symbol))
            {
                continue;
            }

            quxlang::dependencies const& dependencies = graph.make_request< quxlang::direct_dependencies_query >(
                quxlang::direct_dependencies_input{.symbol = symbol, .set = quxlang::dependency_set::native});
            for (quxlang::type_symbol const& placement_root : dependencies.type_placements)
            {
                enqueue_type(placement_root);
            }
            for (quxlang::type_symbol const& struct_root : dependencies.struct_layouts)
            {
                enqueue_type(struct_root);
            }
            for (quxlang::type_symbol const& fusion_root : dependencies.fusion_layouts)
            {
                enqueue_type(fusion_root);
            }
            for (quxlang::type_symbol const& nested_global : dependencies.antestatal_globals)
            {
                enqueue_antestatal_global(nested_global);
            }
            for (quxlang::type_symbol const& global_root : dependencies.global_roots)
            {
                result.global_init_types[global_root] = graph.make_request< quxlang::global_init_type_query >(global_root);
            }

            result.antestatal_constants[symbol] = graph.make_request< quxlang::antestatal_static_value_query >(symbol);
            quxlang::type_symbol const type = graph.make_request< quxlang::variable_type_query >(symbol);
            result.object_reference_types[symbol] = type;
            enqueue_type(type);
        }

        while (!pending_types.empty())
        {
            quxlang::type_symbol type = std::move(pending_types.back());
            pending_types.pop_back();

            bool skip_type_placement_query = false;
            rpnx::apply_visitor< void >(
                type,
                [&](auto const& concrete_type) -> void
                {
                    using concrete_type_value = std::decay_t< decltype(concrete_type) >;

                    if constexpr (std::is_same_v< concrete_type_value, quxlang::nvalue_slot >)
                    {
                        enqueue_type(concrete_type.target);
                        skip_type_placement_query = true;
                    }
                    else if constexpr (std::is_same_v< concrete_type_value, quxlang::dvalue_slot >)
                    {
                        enqueue_type(concrete_type.target);
                        skip_type_placement_query = true;
                    }
                    else if constexpr (std::is_same_v< concrete_type_value, quxlang::attached_type_reference >)
                    {
                        if (!concrete_type.carrying_type.template type_is< quxlang::void_type >())
                        {
                            enqueue_type(concrete_type.carrying_type);
                        }
                    }
                    else if constexpr (std::is_same_v< concrete_type_value, quxlang::ptrref_type >)
                    {
                        enqueue_type(concrete_type.target);
                    }
                    else if constexpr (std::is_same_v< concrete_type_value, quxlang::array_type >)
                    {
                        enqueue_type(concrete_type.element_type);
                    }
                    else if constexpr (std::is_same_v< concrete_type_value, quxlang::array_initializer_type >)
                    {
                        enqueue_type(concrete_type.element_type);
                        skip_type_placement_query = true;
                    }
                    else if constexpr (std::is_same_v< concrete_type_value, quxlang::procedure_type >)
                    {
                        for (quxlang::type_symbol const& positional : concrete_type.signature.params.positional)
                        {
                            enqueue_type(positional);
                        }
                        for (std::pair< std::string const, quxlang::type_symbol > const& named : concrete_type.signature.params.named)
                        {
                            enqueue_type(named.second);
                        }
                        if (concrete_type.signature.return_type.has_value())
                        {
                            enqueue_type(*concrete_type.signature.return_type);
                        }
                        skip_type_placement_query = true;
                    }
                    else if constexpr (std::is_same_v< concrete_type_value, quxlang::storage >)
                    {
                        for (quxlang::type_symbol const& storable_type : concrete_type.storable_types)
                        {
                            enqueue_type(storable_type);
                        }
                    }
                });

            if (std::optional< quxlang::type_symbol > const atomic_value = quxlang::atomic_type_argument(type); atomic_value.has_value())
            {
                enqueue_type(*atomic_value);
            }

            if (type.type_is< quxlang::size_type >())
            {
                result.type_placements[type] = quxlang::class_placement_info{
                    .size = machine.pointer_size_bytes(),
                    .alignment = machine.pointer_align(),
                };
            }
            else if (type.type_is< quxlang::address_type >())
            {
                result.type_placements[type] = quxlang::class_placement_info{
                    .size = machine.pointer_size_bytes(),
                    .alignment = machine.pointer_align(),
                };
            }
            else if (!skip_type_placement_query)
            {
                result.type_placements[type] = graph.make_request< quxlang::class_placement_info_query >(type);
            }

            if (!(type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::subtag_type >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >()))
            {
                continue;
            }

            quxlang::symbol_kind const symbol_kind = graph.make_request< quxlang::symbol_type_query >(type);
            if (symbol_kind == quxlang::symbol_kind::interface_)
            {
                std::vector< quxlang::interface_slot > const slots = graph.make_request< quxlang::interface_slot_list_query >(type);
                std::vector< quxlang::interface_slot_key > slot_keys;
                slot_keys.reserve(slots.size());
                for (quxlang::interface_slot const& slot : slots)
                {
                    slot_keys.push_back(slot.key);
                    for (quxlang::type_symbol const& positional : slot.key.concrete_params.positional)
                    {
                        enqueue_type(positional);
                    }
                    for (std::pair< std::string const, quxlang::type_symbol > const& named : slot.key.concrete_params.named)
                    {
                        enqueue_type(named.second);
                    }
                    if (slot.key.concrete_return_type.has_value())
                    {
                        enqueue_type(*slot.key.concrete_return_type);
                    }
                }
                result.interface_slots[type] = std::move(slot_keys);
                continue;
            }
            quxlang::class_kind const concrete_kind = graph.make_request< quxlang::class_type_query >(type);
            if (concrete_kind == quxlang::class_kind::enum_)
            {
                result.enum_infos[type] = graph.make_request< quxlang::enum_info_query >(type);
                continue;
            }
            if (concrete_kind == quxlang::class_kind::flagset)
            {
                result.flagset_infos[type] = graph.make_request< quxlang::flagset_info_query >(type);
                continue;
            }

            if (concrete_kind == quxlang::class_kind::union_)
            {
                quxlang::union_info const info = graph.make_request< quxlang::union_info_query >(type);
                result.union_infos[type] = info;
                result.fusion_layouts[type] = graph.make_request< quxlang::fusion_layout_query >(type);
                for (quxlang::union_option_info const& option : info.options)
                {
                    enqueue_type(option.type);
                }
                continue;
            }
            if (concrete_kind == quxlang::class_kind::variant)
            {
                quxlang::variant_info const info = graph.make_request< quxlang::variant_info_query >(type);
                result.variant_infos[type] = info;
                result.fusion_layouts[type] = graph.make_request< quxlang::fusion_layout_query >(type);
                for (quxlang::type_symbol const& alternative : info.alternatives)
                {
                    enqueue_type(alternative);
                }
                continue;
            }

            if (concrete_kind != quxlang::class_kind::struct_)
            {
                continue;
            }

            quxlang::struct_layout const layout = graph.make_request< quxlang::struct_layout_query >(type);
            result.struct_layouts[type] = layout;
            for (quxlang::struct_field_info const& field : layout.fields)
            {
                enqueue_type(field.type);
            }
        }

        return result;
    }

    /**
     * Formats a traceback location using source-bundle-relative paths and only the start position.
     */
    auto format_traceback_location(quxlang::vmir2::source_index const* source_index, std::optional< quxlang::source_location > const& location) -> std::string
    {
        if (!location.has_value())
        {
            return {};
        }

        if (source_index != nullptr)
        {
            std::map< std::uint64_t, quxlang::vmir2::indexed_source_file >::const_iterator file_iter = source_index->files.find(location->file_id);
            if (file_iter != source_index->files.end())
            {
                quxlang::vmir2::source_position const begin = file_iter->second.position(location->begin_index);
                return file_iter->second.path() + ":" + std::to_string(begin.line) + ":" + std::to_string(begin.column);
            }
        }

        quxlang::source_location start_location = *location;
        start_location.end_index.reset();
        std::string formatted;
        formatted = quxlang::source_location_suffix(start_location);

        static std::string const source_prefix = " @@ ";
        if (formatted.starts_with(source_prefix))
        {
            formatted.erase(0, source_prefix.size());
        }
        else if (formatted.starts_with(' '))
        {
            formatted.erase(0, 1);
        }

        return formatted;
    }

    /**
     * Prints a qxc compilation error with the active target and source traceback captured by the compiler.
     */
    void print_compilation_error(
        std::ostream& output,
        quxlang::compilation_error const& error,
        quxlang::vmir2::source_index const* source_index,
        std::optional< std::string > const& target_name)
    {
        output << "qxc: compilation error";
        if (target_name.has_value())
        {
            output << " in target '" << *target_name << "'";
        }
        output << ": " << error.what() << '\n';
        if (error.traceback.empty())
        {
            return;
        }

        output << "backtrace:\n";
        for (std::size_t i = 0; i < error.traceback.size(); ++i)
        {
            quxlang::trace_frame const& frame = error.traceback.at(i);
            std::string const location = format_traceback_location(source_index, frame.location);

            output << "  #" << i;
            if (!location.empty())
            {
                output << " " << location;
            }
            else
            {
                output << " <unknown source>";
            }

            if (!frame.trace_context.empty())
            {
                output << " in " << frame.trace_context;
            }
            output << '\n';
        }
    }
} // namespace

int main(int argc, char** argv)
{
    std::optional< quxlang::source_bundle > input_srcs;
    std::optional< quxlang::source_file_index > file_index;
    std::optional< quxlang::vmir2::source_index > source_index;
    std::optional< std::string > active_target_name;

    try
    {
        bool verbose = true;
        if (argc < 3)
        {
            throw quxlang::compilation_error("Usage: qxc <input directory> <output directory> [--debug-compile-output]");
        }

        std::filesystem::path input = argv[1];
        std::filesystem::path output = argv[2];
        bool const debug_compile_output = parse_debug_compile_output(argc, argv);

        input_srcs.emplace(quxlang::load_bundle_sources_for_targets(input, std::nullopt));
        std::cout << "Source bundle BLAKE2b-512: " << source_bundle_hash(*input_srcs) << std::endl;
        file_index.emplace(make_source_file_index(*input_srcs));
        source_index.emplace(*file_index, *input_srcs);

        bool any_failure = false;
        for (auto const& [target_name, target_config] : input_srcs->targets)
        {
            active_target_name = target_name;
            try
            {
                if (verbose)
                {
                    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                    {
                        std::cout << "Compiling Target: " << target_name << std::endl;
                    }
                }

                quxlang::compiler_querygraph graph(*input_srcs, target_name, target_config.target_output_config);

                std::filesystem::path const build_dir = output / "build" / target_name;
                std::filesystem::path const output_dir = output / "output" / target_name;

                std::filesystem::create_directories(output_dir);
                try
                {
                    std::map< std::string, std::vector<std::byte> > const artifacts =
                        graph.make_request< quxlang::output_binary_artifacts_query >(std::monostate{});

                    for (std::pair< std::string const, std::vector<std::byte> > const& artifact_entry : artifacts)
                    {
                        std::filesystem::path const executable_path = write_final_output_file(output_dir, artifact_entry.first, artifact_entry.second);
                        std::cout << "Output binary BLAKE2b-512 (" << target_name << "/" << artifact_entry.first << "): "
                                  << quxlang::blake2b::hex(artifact_entry.second) << std::endl;
                        if (verbose)
                        {
                            std::cout << "Wrote output executable: " << target_name << "/" << artifact_entry.first << " -> " << executable_path.string() << std::endl;
                        }
                    }
                }
                catch (quxlang::compilation_error const& error)
                {
                    print_compilation_error(std::cerr, error, source_index.has_value() ? &*source_index : nullptr, active_target_name);
                    any_failure = true;
                }

                if (!debug_compile_output)
                {
                    active_target_name.reset();
                    continue;
                }

                std::filesystem::create_directories(output_dir);

                std::map< std::string, quxlang::output_query_output > const outputs_to_compile =
                    graph.make_request< quxlang::output_binaries_information_query >(std::monostate{});
                std::set< quxlang::type_symbol > compiled_vmir2_routines;
                std::set< quxlang::type_symbol > compiled_llvm_routines;
                std::set< quxlang::type_symbol > compiled_asm_routines;
                quxlang::llvm_backend::llvm_backend llvm_backend;
                auto collect_routine_tree =
                    [&](quxlang::type_symbol root_symbol,
                        quxlang::vmir2::functanoid_routine3 const& root_routine,
                        std::vector< quxlang::trace_frame > root_traceback,
                        std::optional< quxlang::type_symbol > runtime_program_start) -> collected_routine_tree
                {
                    std::vector< std::pair< quxlang::instanciation_reference, std::vector< quxlang::trace_frame > > > pending_functanoids;
                    std::set< quxlang::type_symbol > queued_functanoids;
                    std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 > tree_routines;
                    std::map< quxlang::type_symbol, quxlang::asm_callable > asm_callable_interfaces;
                    std::map< quxlang::type_symbol, quxlang::asm_procedure > asm_routines;
                    std::map< quxlang::llvm_backend::runtime_procedure_reference, quxlang::type_symbol > runtime_procedures;
                    std::set< quxlang::type_symbol > object_references;
                    std::set< quxlang::type_symbol > queued_antestatal_globals;
                    std::vector< quxlang::type_symbol > pending_antestatal_globals;
                    quxlang::qxc_detail::llvm_inlining_dependency_graph dependency_graph;
                    tree_routines.emplace(root_symbol, root_routine);

                    struct pending_runtime_procedure_entry
                    {
                        quxlang::llvm_backend::runtime_procedure_reference reference;
                        quxlang::type_symbol caller;
                        std::vector< quxlang::trace_frame > traceback;
                    };

                    auto enqueue_functanoid = [&](quxlang::instanciation_reference const& functanoid, std::vector< quxlang::trace_frame > traceback) -> void
                    {
                        quxlang::type_symbol functanoid_symbol = functanoid;
                        if (!queued_functanoids.insert(functanoid_symbol).second)
                        {
                            return;
                        }
                        pending_functanoids.push_back(std::make_pair(functanoid, std::move(traceback)));
                    };
                    auto enqueue_antestatal_global = [&](quxlang::type_symbol const& symbol) -> void
                    {
                        if (queued_antestatal_globals.insert(symbol).second)
                        {
                            pending_antestatal_globals.push_back(symbol);
                        }
                    };
                    std::vector< pending_runtime_procedure_entry > pending_runtime_procedures;
                    auto enqueue_runtime_procedure =
                        [&](quxlang::llvm_backend::runtime_procedure_reference const& reference,
                            quxlang::type_symbol const& caller,
                            std::vector< quxlang::trace_frame > traceback) -> void
                    {
                        pending_runtime_procedures.push_back(pending_runtime_procedure_entry{
                            .reference = reference,
                            .caller = caller,
                            .traceback = std::move(traceback),
                        });
                    };
                    auto enqueue_runtime_procedure_dependencies =
                        [&](quxlang::type_symbol const& caller,
                            std::vector< quxlang::trace_frame > const& traceback) -> void
                    {
                        runtime_procedure_reference_locations const referenced_runtime_procedures = directly_referenced_runtime_procedure_locations(graph, caller);
                        for (std::pair< quxlang::llvm_backend::runtime_procedure_reference const, std::optional< quxlang::source_location > > const& referenced_runtime_procedure : referenced_runtime_procedures)
                        {
                            enqueue_runtime_procedure(
                                referenced_runtime_procedure.first,
                                caller,
                                make_dependency_traceback(traceback, caller, referenced_runtime_procedure.second));
                        }
                    };

                    auto collect_asm_routine =
                        [&](quxlang::type_symbol const& asm_symbol,
                            std::vector< quxlang::trace_frame > dependency_traceback) -> void
                    {
                        quxlang::type_symbol asm_body_symbol = asm_symbol;
                        if (asm_symbol.type_is< quxlang::instanciation_reference >())
                        {
                            asm_body_symbol = asm_symbol.get_as< quxlang::instanciation_reference >().temploid.templexoid;
                        }

                        if (asm_symbol.type_is< quxlang::instanciation_reference >() && !asm_callable_interfaces.contains(asm_symbol))
                        {
                            quxlang::asm_procedure selected_procedure = graph.make_request< quxlang::asm_procedure_from_symbol_query >(asm_symbol);
                            if (!selected_procedure.callable_interface.has_value())
                            {
                                throw quxlang::compiler_bug("Selected asm procedure has no callable interface: " + quxlang::to_string(asm_symbol));
                            }
                            asm_callable_interfaces.emplace(asm_symbol, *selected_procedure.callable_interface);
                            dependency_graph[asm_symbol].insert(asm_body_symbol);
                        }

                        if (asm_routines.contains(asm_body_symbol))
                        {
                            return;
                        }

                        quxlang::asm_procedure procedure = graph.make_request< quxlang::asm_procedure_from_symbol_query >(asm_body_symbol);
                        asm_routines.emplace(asm_body_symbol, std::move(procedure));

                        quxlang::dependencies const& direct_dependencies = graph.make_request< quxlang::direct_dependencies_query >(
                            quxlang::direct_dependencies_input{.symbol = asm_body_symbol, .set = quxlang::dependency_set::native});
                        functanoid_reference_locations referenced_functanoids;
                        for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& dependency : direct_dependencies.functanoids)
                        {
                            record_referenced_functanoid(graph, referenced_functanoids, dependency.first, dependency.second);
                        }
                        for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : referenced_functanoids)
                        {
                            quxlang::type_symbol const& referenced_symbol = referenced_functanoid.first;
                            if (!referenced_symbol.type_is< quxlang::instanciation_reference >())
                            {
                                throw quxlang::compiler_bug("ASM dependency scan returned a non-instanciation reference: " + quxlang::to_string(referenced_symbol));
                            }
                            enqueue_functanoid(
                                referenced_symbol.as< quxlang::instanciation_reference >(),
                                make_dependency_traceback(dependency_traceback, asm_body_symbol, referenced_functanoid.second));
                        }
                        for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : referenced_functanoids)
                        {
                            dependency_graph[asm_body_symbol].insert(referenced_functanoid.first);
                        }

                        for (quxlang::type_symbol const& referenced_object : direct_dependencies.global_roots)
                        {
                            object_references.insert(referenced_object);
                            if (!is_main_function_object_symbol(referenced_object) &&
                                graph.make_request< quxlang::symbol_type_query >(referenced_object) != quxlang::symbol_kind::global_variable)
                            {
                                throw quxlang::semantic_compilation_error("OBJECT_REF target is not a global object: " + quxlang::to_string(referenced_object));
                            }
                            if (!is_main_function_object_symbol(referenced_object) &&
                                !quxlang::llvm_backend::unit_test_object_type(referenced_object).has_value() &&
                                graph.make_request< quxlang::global_is_antestatal_static_query >(referenced_object))
                            {
                                enqueue_antestatal_global(referenced_object);
                            }
                        }
                    };

                    functanoid_reference_locations const root_dependencies = directly_referenced_functanoid_locations(graph, root_symbol);
                    for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : root_dependencies)
                    {
                        quxlang::type_symbol const& referenced_symbol = referenced_functanoid.first;
                        if (!referenced_symbol.type_is< quxlang::instanciation_reference >())
                        {
                            throw quxlang::compiler_bug("VMIR2 dependency scan returned a non-instanciation reference: " + quxlang::to_string(referenced_symbol));
                        }
                        enqueue_functanoid(
                            referenced_symbol.as< quxlang::instanciation_reference >(),
                            make_dependency_traceback(root_traceback, root_symbol, referenced_functanoid.second));
                    }
                    for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : root_dependencies)
                    {
                        dependency_graph[root_symbol].insert(referenced_functanoid.first);
                    }
                    enqueue_runtime_procedure_dependencies(root_symbol, root_traceback);
                    quxlang::dependencies const& root_direct_dependencies = graph.make_request< quxlang::direct_dependencies_query >(
                        quxlang::direct_dependencies_input{.symbol = root_symbol, .set = quxlang::dependency_set::native});
                    for (quxlang::type_symbol const& global : root_direct_dependencies.antestatal_globals)
                    {
                        enqueue_antestatal_global(global);
                    }

                    if (runtime_program_start.has_value())
                    {
                        quxlang::ast2_symboid const symboid = graph.make_request< quxlang::symboid_query >(*runtime_program_start);
                        if (!symboid.type_is< quxlang::ast2_asm_procedure_declaration >())
                        {
                            throw quxlang::compiler_bug("Runtime PROGRAM_START was not an ASM_PROCEDURE after qxc resolved it");
                        }
                        collect_asm_routine(*runtime_program_start, {});
                    }

                    while (!pending_functanoids.empty() || !pending_runtime_procedures.empty() || !pending_antestatal_globals.empty())
                    {
                        while (!pending_antestatal_globals.empty())
                        {
                            quxlang::type_symbol global = std::move(pending_antestatal_globals.back());
                            pending_antestatal_globals.pop_back();

                            quxlang::dependencies const& direct_dependencies = graph.make_request< quxlang::direct_dependencies_query >(
                                quxlang::direct_dependencies_input{.symbol = global, .set = quxlang::dependency_set::native});
                            functanoid_reference_locations const referenced_functanoids = directly_referenced_functanoid_locations(graph, global);
                            for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : referenced_functanoids)
                            {
                                if (!referenced_functanoid.first.type_is< quxlang::instanciation_reference >())
                                {
                                    throw quxlang::compiler_bug("Antestatal dependency scan returned a non-instanciation reference: " + quxlang::to_string(referenced_functanoid.first));
                                }
                                enqueue_functanoid(
                                    referenced_functanoid.first.as< quxlang::instanciation_reference >(),
                                    make_dependency_traceback({}, global, referenced_functanoid.second));
                            }
                            enqueue_runtime_procedure_dependencies(global, {});
                            for (quxlang::type_symbol const& nested_global : direct_dependencies.antestatal_globals)
                            {
                                enqueue_antestatal_global(nested_global);
                            }
                        }

                        while (!pending_runtime_procedures.empty())
                        {
                            pending_runtime_procedure_entry pending_runtime_procedure = std::move(pending_runtime_procedures.back());
                            pending_runtime_procedures.pop_back();

                            std::map< quxlang::llvm_backend::runtime_procedure_reference, quxlang::type_symbol >::const_iterator const existing_runtime =
                                runtime_procedures.find(pending_runtime_procedure.reference);
                            quxlang::type_symbol runtime_symbol;
                            if (existing_runtime == runtime_procedures.end())
                            {
                                runtime_symbol = resolve_runtime_procedure_symbol(graph, target_config, pending_runtime_procedure.reference);
                                runtime_procedures.emplace(pending_runtime_procedure.reference, runtime_symbol);
                            }
                            else
                            {
                                runtime_symbol = existing_runtime->second;
                            }

                            dependency_graph[pending_runtime_procedure.caller].insert(runtime_symbol);
                            if (!runtime_symbol.type_is< quxlang::instanciation_reference >())
                            {
                                throw quxlang::compiler_bug("Runtime procedure did not resolve to a functanoid: " + quxlang::to_string(runtime_symbol));
                            }
                            enqueue_functanoid(runtime_symbol.as< quxlang::instanciation_reference >(), std::move(pending_runtime_procedure.traceback));
                        }
                        if (pending_functanoids.empty())
                        {
                            continue;
                        }

                        std::pair< quxlang::instanciation_reference, std::vector< quxlang::trace_frame > > pending_functanoid = std::move(pending_functanoids.back());
                        pending_functanoids.pop_back();

                        quxlang::instanciation_reference functanoid = std::move(pending_functanoid.first);
                        std::vector< quxlang::trace_frame > dependency_traceback = std::move(pending_functanoid.second);
                        quxlang::type_symbol functanoid_symbol = functanoid;
                        if (tree_routines.contains(functanoid_symbol) || asm_callable_interfaces.contains(functanoid_symbol))
                        {
                            continue;
                        }

                        if (verbose)
                        {
                            std::cout << "Compiling VMIR2 function: " << quxlang::to_string(functanoid_symbol) << std::endl;
                        }

                        quxlang::type_symbol asm_declaration_symbol = functanoid_symbol;
                        if (functanoid_symbol.type_is< quxlang::instanciation_reference >())
                        {
                            asm_declaration_symbol = functanoid_symbol.get_as< quxlang::instanciation_reference >().temploid.templexoid;
                        }

                        quxlang::ast2_symboid const symboid = graph.make_request< quxlang::symboid_query >(asm_declaration_symbol);
                        if (symboid.type_is< quxlang::ast2_asm_procedure_declaration >())
                        {
                            collect_asm_routine(functanoid_symbol, std::move(dependency_traceback));
                            continue;
                        }
                        else if (symboid.type_is< quxlang::ast2_extern_procedure >())
                        {
                            continue;
                        }

                        quxlang::vmir2::functanoid_routine3 procedure;
                        try
                        {
                            procedure = graph.make_request< quxlang::vm_procedure3_query >(functanoid);
                        }
                        catch (quxlang::compilation_error& error)
                        {
                            error.traceback.insert(error.traceback.end(), dependency_traceback.begin(), dependency_traceback.end());
                            throw;
                        }

                        tree_routines.emplace(functanoid_symbol, procedure);

                        functanoid_reference_locations const referenced_functanoids = directly_referenced_functanoid_locations(graph, functanoid_symbol);
                        for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : referenced_functanoids)
                        {
                            quxlang::type_symbol const& referenced_symbol = referenced_functanoid.first;
                            if (!referenced_symbol.type_is< quxlang::instanciation_reference >())
                            {
                                throw quxlang::compiler_bug("VMIR2 dependency scan returned a non-instanciation reference: " + quxlang::to_string(referenced_symbol));
                            }
                            enqueue_functanoid(
                                referenced_symbol.as< quxlang::instanciation_reference >(),
                                make_dependency_traceback(dependency_traceback, functanoid_symbol, referenced_functanoid.second));
                        }
                        for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : referenced_functanoids)
                        {
                            dependency_graph[functanoid_symbol].insert(referenced_functanoid.first);
                        }
                        enqueue_runtime_procedure_dependencies(functanoid_symbol, dependency_traceback);
                        quxlang::dependencies const& direct_dependencies = graph.make_request< quxlang::direct_dependencies_query >(
                            quxlang::direct_dependencies_input{.symbol = functanoid_symbol, .set = quxlang::dependency_set::native});
                        for (quxlang::type_symbol const& global : direct_dependencies.antestatal_globals)
                        {
                            enqueue_antestatal_global(global);
                        }
                    }

                    collected_routine_tree result;
                    result.routines = std::move(tree_routines);
                    result.asm_callable_interfaces = std::move(asm_callable_interfaces);
                    result.asm_routines = std::move(asm_routines);
                    result.runtime_procedures = std::move(runtime_procedures);
                    result.object_references = std::move(object_references);
                    result.dependency_graph = std::move(dependency_graph);
                    result.support = build_llvm_packet_support_data(graph, target_config.target_output_config, result.routines, result.object_references);
                    for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& routine_entry : result.routines)
                    {
                        quxlang::ast2_procedure_ref procedure_ref{.cc = "", .functanoid = routine_entry.first};
                        result.support.procedure_linksymbols.emplace(routine_entry.first, graph.make_request< quxlang::procedure_linksymbol_query >(procedure_ref));
                    }
                    for (std::pair< quxlang::type_symbol const, quxlang::asm_procedure > const& routine_entry : result.asm_routines)
                    {
                        quxlang::ast2_procedure_ref procedure_ref{.cc = "", .functanoid = routine_entry.first};
                        result.support.procedure_linksymbols.emplace(routine_entry.first, graph.make_request< quxlang::procedure_linksymbol_query >(procedure_ref));
                    }
                    for (std::pair< quxlang::type_symbol const, quxlang::asm_callable > const& routine_entry : result.asm_callable_interfaces)
                    {
                        quxlang::type_symbol asm_body_symbol = routine_entry.first;
                        if (routine_entry.first.type_is< quxlang::instanciation_reference >())
                        {
                            asm_body_symbol = routine_entry.first.get_as< quxlang::instanciation_reference >().temploid.templexoid;
                        }
                        quxlang::ast2_procedure_ref procedure_ref{.cc = "", .functanoid = asm_body_symbol};
                        result.support.procedure_linksymbols.emplace(routine_entry.first, graph.make_request< quxlang::procedure_linksymbol_query >(procedure_ref));
                    }
                    return result;
                };

                auto emit_routine_tree =
                    [&](collected_routine_tree const& tree,
                        quxlang::llvm_backend::optimization_level const optimization_level) -> void
                {
                    for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& routine_entry : tree.routines)
                    {
                        quxlang::type_symbol const& routine_symbol = routine_entry.first;
                        quxlang::vmir2::functanoid_routine3 const& routine = routine_entry.second;
                        if (compiled_vmir2_routines.contains(routine_symbol))
                        {
                            continue;
                        }

                        quxlang::vmir2::assembler assembler(routine, *source_index);
                        std::string const vmir_text = assembler.to_string(routine);
                        std::filesystem::path const vmir_path = write_vmir2_text_file(build_dir, routine_symbol, vmir_text);
                        if (verbose)
                        {
                            std::cout << "Wrote VMIR2: " << quxlang::to_string(routine_symbol) << " -> " << vmir_path.string() << std::endl;
                        }
                        compiled_vmir2_routines.insert(routine_symbol);
                    }

                    for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& routine_entry : tree.routines)
                    {
                        quxlang::type_symbol const& routine_symbol = routine_entry.first;
                        quxlang::vmir2::functanoid_routine3 const& routine = routine_entry.second;
                        if (compiled_llvm_routines.contains(routine_symbol))
                        {
                            continue;
                        }

                        quxlang::llvm_backend::llvm_compilable_unit compilable_unit;
                        compilable_unit.target_name = routine_symbol;
                        compilable_unit.target_code = routine;
                        compilable_unit.machine_target.machine = target_config.target_output_config;
                        compilable_unit.machine_target.optimization = optimization_level;
                        compilable_unit.source_index = rpnx::cow< quxlang::vmir2::source_index >(*source_index);
                        compilable_unit.procedure_linksymbols = tree.support.procedure_linksymbols;
                        compilable_unit.runtime_procedures = tree.runtime_procedures;
                        compilable_unit.object_reference_types = tree.support.object_reference_types;
                        compilable_unit.antestatal_constants = tree.support.antestatal_constants;
                        compilable_unit.global_init_types = tree.support.global_init_types;
                        compilable_unit.interface_slots = tree.support.interface_slots;
                        compilable_unit.enum_infos = tree.support.enum_infos;
                        compilable_unit.flagset_infos = tree.support.flagset_infos;
                        compilable_unit.struct_layouts = tree.support.struct_layouts;
                        compilable_unit.union_infos = tree.support.union_infos;
                        compilable_unit.variant_infos = tree.support.variant_infos;
                        compilable_unit.fusion_layouts = tree.support.fusion_layouts;
                        compilable_unit.type_placements = tree.support.type_placements;

                        std::set< quxlang::type_symbol > const inlinable_symbols =
                            quxlang::qxc_detail::collect_potentially_inlinable_functanoids(tree.dependency_graph, routine_symbol);

                        for (quxlang::type_symbol const& helper_symbol : inlinable_symbols)
                        {
                            std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 >::const_iterator helper_iter = tree.routines.find(helper_symbol);
                            if (helper_iter == tree.routines.end())
                            {
                                continue;
                            }
                            compilable_unit.inlinable_functions.insert(*helper_iter);
                        }
                        for (quxlang::type_symbol const& helper_symbol : inlinable_symbols)
                        {
                            std::map< quxlang::type_symbol, quxlang::asm_callable >::const_iterator helper_iter = tree.asm_callable_interfaces.find(helper_symbol);
                            if (helper_iter == tree.asm_callable_interfaces.end())
                            {
                                continue;
                            }
                            compilable_unit.asm_callable_interfaces.insert(*helper_iter);
                        }
                        for (quxlang::type_symbol const& helper_symbol : inlinable_symbols)
                        {
                            std::map< quxlang::type_symbol, quxlang::asm_procedure >::const_iterator helper_iter = tree.asm_routines.find(helper_symbol);
                            if (helper_iter == tree.asm_routines.end())
                            {
                                continue;
                            }
                            compilable_unit.asm_functions.insert(*helper_iter);
                        }

                        quxlang::llvm_backend::llvm_compiled_unit const llvm_unit = llvm_backend.compile(compilable_unit);
                        std::filesystem::path const input_llvm_path = write_input_llvm_text_file(build_dir, routine_symbol, llvm_unit.llvm_ir_text);
                        std::filesystem::path const final_llvm_path = write_final_llvm_text_file(build_dir, routine_symbol, llvm_unit.optimized_llvm_ir_text);
                        std::filesystem::path const input_object_path = write_input_object_file(build_dir, routine_symbol, llvm_unit.object_file);
                        std::filesystem::path const final_object_path = write_final_object_file(build_dir, routine_symbol, llvm_unit.optimized_object_file);
                        if (verbose)
                        {
                            std::cout << "Wrote input functanoid LLVM: " << quxlang::to_string(routine_symbol) << " -> " << input_llvm_path.string() << std::endl;
                            std::cout << "Wrote final functanoid LLVM: " << quxlang::to_string(routine_symbol) << " -> " << final_llvm_path.string() << std::endl;
                            std::cout << "Wrote input functanoid LLVM object: " << quxlang::to_string(routine_symbol) << " -> " << input_object_path.string() << std::endl;
                            std::cout << "Wrote final functanoid LLVM object: " << quxlang::to_string(routine_symbol) << " -> " << final_object_path.string() << std::endl;
                        }

                        compiled_llvm_routines.insert(routine_symbol);
                    }

                    for (std::pair< quxlang::type_symbol const, quxlang::asm_procedure > const& routine_entry : tree.asm_routines)
                    {
                        quxlang::type_symbol const& routine_symbol = routine_entry.first;
                        if (compiled_asm_routines.contains(routine_symbol))
                        {
                            continue;
                        }

                        quxlang::llvm_backend::llvm_assembled_procedure const assembled =
                            llvm_backend.assemble(
                                quxlang::llvm_backend::llvm_compilation_target{
                                    .machine = target_config.target_output_config,
                                    .optimization = optimization_level,
                                },
                                routine_entry.second);
                        std::filesystem::path const asm_path = write_asm_source_file(build_dir, routine_symbol, assembled.assembly_text);
                        std::filesystem::path const asm_object_path = write_asm_object_file(build_dir, routine_symbol, assembled.object_file);
                        if (verbose)
                        {
                            std::cout << "Wrote asm source: " << quxlang::to_string(routine_symbol) << " -> " << asm_path.string() << std::endl;
                            std::cout << "Wrote asm object: " << quxlang::to_string(routine_symbol) << " -> " << asm_object_path.string() << std::endl;
                        }

                        compiled_asm_routines.insert(routine_symbol);
                    }
                };

                for (auto const& [module_name, _] : target_config.module_configurations)
                {
                    quxlang::type_symbol const module_symbol = quxlang::absolute_module_reference{.module_name = module_name};
                    std::set< quxlang::type_symbol > const static_tests = graph.make_request< quxlang::list_static_tests_query >(module_symbol);
                    for (quxlang::type_symbol const& static_test_symbol : static_tests)
                    {
                        auto sym = graph.make_request< quxlang::symboid_query >(static_test_symbol);
                        if (!sym.type_is< quxlang::ast2_test >())
                        {
                            throw quxlang::compiler_bug("list_static_tests returned a non-static-test symbol: " + quxlang::to_string(static_test_symbol));
                        }

                        quxlang::ast2_test const& static_test_decl = sym.get_as< quxlang::ast2_test >();
                        if (static_test_decl.expected_mode == quxlang::static_test_expected_mode::expect_compilation_failure)
                        {
                            continue;
                        }

                        if (verbose)
                        {
                            std::cout << "Compiling STATIC_TEST VMIR2: " << quxlang::to_string(static_test_symbol) << std::endl;
                        }

                        quxlang::vmir2::functanoid_routine3 static_test_routine = graph.make_request< quxlang::static_test_vmir_query >(static_test_symbol);
                        emit_routine_tree(
                            collect_routine_tree(static_test_symbol, static_test_routine, {}, std::nullopt),
                            configured_llvm_optimization_level(target_config.llvm_options));
                    }
                }

                for (std::pair< std::string const, quxlang::output_query_output > const& output_pair : outputs_to_compile)
                {
                    quxlang::output_query_output const& output_entry = output_pair.second;
                    if (!target_config.module_configurations.contains(output_entry.module_name))
                    {
                        throw quxlang::semantic_compilation_error("Target '" + target_name + "' output '" + output_entry.output_name + "' references unknown module '" + output_entry.module_name + "'");
                    }

                    if (verbose)
                    {
                        std::cout << "Compiling VMIR2 output: " << target_name << "/" << output_entry.output_name << std::endl;
                    }

                    quxlang::backend_llvm_options const output_llvm_options =
                        graph.make_request< quxlang::output_llvm_backend_options_query >(output_entry.output_name);

                    if (output_entry.type == quxlang::output_kind::unit_test_suite)
                    {
                        quxlang::type_symbol const module_symbol = quxlang::absolute_module_reference{.module_name = output_entry.module_name};
                        std::set< quxlang::type_symbol > const unit_tests = graph.make_request< quxlang::list_unit_tests_query >(module_symbol);
                        for (quxlang::type_symbol const& unit_test_symbol : unit_tests)
                        {
                            if (verbose)
                            {
                                std::cout << "Compiling UNIT_TEST VMIR2: " << quxlang::to_string(unit_test_symbol) << std::endl;
                            }

                            quxlang::vmir2::functanoid_routine3 unit_test_routine = graph.make_request< quxlang::unit_test_vmir_query >(unit_test_symbol);
                            emit_routine_tree(
                                collect_routine_tree(unit_test_symbol, unit_test_routine, {}, std::nullopt),
                                configured_llvm_optimization_level(output_llvm_options));
                        }
                    }
                    else
                    {
                        if (!output_entry.main_functanoid.has_value())
                        {
                            throw quxlang::semantic_compilation_error("Output '" + output_entry.output_name + "' requires a main functanoid");
                        }

                        quxlang::instanciation_reference entry_functanoid = resolve_entry_functanoid(graph, output_entry.module_name, *output_entry.main_functanoid);
                        if (output_entry.type == quxlang::output_kind::executable)
                        {
                            validate_executable_entry_signature(graph, entry_functanoid);
                        }
                        quxlang::vmir2::functanoid_routine3 entry_routine = graph.make_request< quxlang::vm_procedure3_query >(entry_functanoid);
                        std::optional< quxlang::type_symbol > const runtime_program_start =
                            try_resolve_runtime_entrypoint(graph, target_config, runtime_start_name(output_entry.type));
                        collected_routine_tree const output_tree = collect_routine_tree(entry_functanoid, entry_routine, {}, runtime_program_start);
                        emit_routine_tree(output_tree, configured_llvm_optimization_level(output_llvm_options));
                    }

                    quxlang::llvm_backend::llvm_compilable_unit const output_packet = graph.make_request< quxlang::output_llvm_input_query >(output_entry.output_name);
                    std::string const input_output_module_llvm = graph.make_request< quxlang::output_unoptimized_llvm_query >(output_entry.output_name);
                    std::string const final_output_module_llvm = graph.make_request< quxlang::output_optimized_llvm_query >(output_entry.output_name);
                    quxlang::llvm_backend::llvm_compiled_unit const output_module = graph.make_request< quxlang::llvm_compiled_output_query >(output_entry.output_name);
                    std::filesystem::path const input_output_module_path =
                        write_output_module_input_llvm_text_file(build_dir, output_entry.output_name, output_packet.target_name, input_output_module_llvm);
                    std::filesystem::path const final_output_module_path =
                        write_output_module_final_llvm_text_file(build_dir, output_entry.output_name, output_packet.target_name, final_output_module_llvm);
                    std::filesystem::path const input_output_module_object_path =
                        write_output_module_input_object_file(build_dir, output_entry.output_name, output_module.object_file);
                    std::filesystem::path const final_output_module_object_path =
                        write_output_module_final_object_file(build_dir, output_entry.output_name, output_module.optimized_object_file);
                    if (verbose)
                    {
                        std::cout << "Wrote input output-module LLVM: " << target_name << "/" << output_entry.output_name << " -> " << input_output_module_path.string() << std::endl;
                        std::cout << "Wrote final output-module LLVM: " << target_name << "/" << output_entry.output_name << " -> " << final_output_module_path.string() << std::endl;
                        std::cout << "Wrote input output-module LLVM object: " << target_name << "/" << output_entry.output_name << " -> " << input_output_module_object_path.string() << std::endl;
                        std::cout << "Wrote final output-module LLVM object: " << target_name << "/" << output_entry.output_name << " -> " << final_output_module_object_path.string() << std::endl;
                    }

                }
                active_target_name.reset();
            }
            catch (quxlang::compilation_error const& error)
            {
                print_compilation_error(std::cerr, error, source_index.has_value() ? &*source_index : nullptr, active_target_name);
                any_failure = true;
                active_target_name.reset();
            }
        }

        return any_failure ? 1 : 0;
    }
    catch (quxlang::compilation_error const& error)
    {
        print_compilation_error(std::cerr, error, source_index.has_value() ? &*source_index : nullptr, active_target_name);
        return 1;
    }
}
