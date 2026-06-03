// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/compiler_querygraph.hpp"
#include "quxlang/llvm-backend.hpp"
#include "quxlang/manipulators/mangler.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"
#include "quxlang/queries/asm_procedure_from_symbol.hpp"
#include "quxlang/queries/antestatal_static_value.hpp"
#include "quxlang/queries/class_layout.hpp"
#include "quxlang/queries/enum_info.hpp"
#include "quxlang/queries/flagset_info.hpp"
#include "quxlang/queries/global_is_antestatal_static.hpp"
#include "quxlang/queries/interface_slot_list.hpp"
#include "quxlang/queries/instanciation.hpp"
#include "quxlang/queries/list_static_tests.hpp"
#include "quxlang/queries/lookup.hpp"
#include "quxlang/queries/procedure_linksymbol.hpp"
#include "quxlang/queries/static_test_vmir.hpp"
#include "quxlang/queries/symboid.hpp"
#include "quxlang/queries/symbol_type.hpp"
#include "quxlang/queries/temploid_formal_ensig.hpp"
#include "quxlang/queries/type_placement_info.hpp"
#include "quxlang/queries/variable_type.hpp"
#include "quxlang/queries/vm_procedure3.hpp"
#include "quxlang/linker/elf_linker.hpp"
#include "quxlang/source_loader.hpp"
#include "quxlang/vmir2/assembler.hpp"
#include "quxlang/vmir2/routine_requirements.hpp"
#include "quxlang/vmir2/source_index.hpp"
#include "qxc_llvm_inlining.hpp"
#include "qxc_output_paths.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
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
     * Returns true when a declared overload still contains template-only
     * parameter types and cannot be emitted as one concrete VMIR2 routine.
     */
    auto overload_has_unspecialized_parameters(quxlang::temploid_ensig const& ensig) -> bool
    {
        for (quxlang::argif const& param : ensig.interface.positional)
        {
            if (quxlang::is_template(param.type))
            {
                return true;
            }
        }

        for (auto const& [_, param] : ensig.interface.named)
        {
            if (quxlang::is_template(param.type))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * Builds a concrete instantiation parameter set from a selected overload.
     */
    auto instantiate_declared_overload(quxlang::temploid_ensig const& ensig) -> quxlang::instatype
    {
        quxlang::instatype result;
        for (quxlang::argif const& param : ensig.interface.positional)
        {
            if (!param.is_pack)
            {
                result.positional.push_back(quxlang::make_type_instantiation(param.type));
            }
        }

        for (auto const& [name, param] : ensig.interface.named)
        {
            result.named[name] = quxlang::make_type_instantiation(param.type);
        }

        return result;
    }

    /**
     * Parses a type-symbol expression used by qxc configuration.
     */
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

    /**
     * Records a concrete functanoid dependency and the source location that referenced it.
     */
    void record_referenced_functanoid(quxlang::compiler_querygraph& graph, functanoid_reference_locations& result, quxlang::type_symbol const& symbol, std::optional< quxlang::source_location > location)
    {
        std::optional< quxlang::instanciation_reference > concrete = concrete_functanoid_from_symbol(graph, symbol);
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
     * Finds direct functanoid dependencies of a VMIR2 routine and keeps source locations for invoke-like references.
     */
    auto directly_referenced_functanoid_locations(quxlang::compiler_querygraph& graph, quxlang::vmir2::functanoid_routine3 const& routine) -> functanoid_reference_locations
    {
        functanoid_reference_locations result;

        for (auto const& [_, dtor] : routine.non_trivial_dtors)
        {
            record_referenced_functanoid(graph, result, dtor, std::nullopt);
        }

        for (quxlang::vmir2::executable_block const& block : routine.blocks)
        {
            for (auto const& [_, slot] : block.entry_state)
            {
                if (slot.nontrivial_dtor.has_value())
                {
                    record_referenced_functanoid(graph, result, slot.nontrivial_dtor->func, std::nullopt);
                }
            }

            for (quxlang::vmir2::vm_instruction const& instruction : block.instructions)
            {
                std::optional< quxlang::source_location > const location = quxlang::vmir2::get_location(instruction);
                rpnx::apply_visitor< void >(
                    instruction,
                    [&](auto const& concrete_instruction) -> void
                    {
                        using instruction_type = std::decay_t< decltype(concrete_instruction) >;

                        if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::invoke >)
                        {
                            record_referenced_functanoid(graph, result, concrete_instruction.what, location);
                        }
                        else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::defer_nontrivial_dtor >)
                        {
                            record_referenced_functanoid(graph, result, concrete_instruction.func, location);
                        }
                        else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::get_procedure_ptr >)
                        {
                            record_referenced_functanoid(graph, result, concrete_instruction.routine, location);
                        }
                        else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::interface_init >)
                        {
                            for (auto const& [_, routine_symbol] : concrete_instruction.functions)
                            {
                                record_referenced_functanoid(graph, result, routine_symbol, location);
                            }
                        }
                        else if constexpr (std::is_same_v< instruction_type, quxlang::vmir2::interface_invoke >)
                        {
                            if (concrete_instruction.default_function.has_value())
                            {
                                record_referenced_functanoid(graph, result, *concrete_instruction.default_function, location);
                            }
                        }
                    });
            }
        }

        return result;
    }

    /**
     * Finds direct functanoid dependencies referenced from one parsed assembly routine declaration.
     */
    auto directly_referenced_functanoid_locations(quxlang::compiler_querygraph& graph, quxlang::ast2_asm_procedure_declaration const& routine)
        -> functanoid_reference_locations
    {
        functanoid_reference_locations result;

        for (quxlang::ast2_asm_instruction const& instruction : routine.instructions)
        {
            for (quxlang::ast2_asm_operand const& operand : instruction.operands)
            {
                for (quxlang::ast2_asm_operand_component const& component : operand.components)
                {
                    if (component.type_is< quxlang::ast2_procedure_ref >())
                    {
                        quxlang::ast2_procedure_ref const& procedure_ref = component.get_as< quxlang::ast2_procedure_ref >();
                        record_referenced_functanoid(graph, result, procedure_ref.functanoid, std::nullopt);
                    }
                }
            }
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
    auto resolve_entry_functanoid(quxlang::compiler_querygraph& graph, std::string const& module_name, std::string const& main_functanoid_text)
        -> quxlang::instanciation_reference
    {
        quxlang::type_symbol const module_context = quxlang::absolute_module_reference{.module_name = module_name};
        quxlang::type_symbol parsed_entry = parse_type_symbol_text(main_functanoid_text);
        quxlang::type_symbol contextual_entry = quxlang::with_context(parsed_entry, module_context);

        auto resolved_entry = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
            .context = module_context,
            .type = contextual_entry,
        });

        if (!resolved_entry.has_value())
        {
            throw quxlang::semantic_compilation_error("Could not resolve main functanoid '" + main_functanoid_text + "' in module '" + module_name + "'");
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
            throw quxlang::semantic_compilation_error("Main functanoid '" + main_functanoid_text + "' in module '" + module_name + "' is not callable as a concrete function");
        }

        return *instanciation;
    }

    /**
     * Finds the optional runtime-provided Linux process entrypoint declaration for one target.
     */
    auto try_resolve_runtime_program_start(quxlang::compiler_querygraph& graph, quxlang::target_configuration const& target_config) -> std::optional< quxlang::type_symbol >
    {
        if (!target_config.module_configurations.contains("RUNTIME"))
        {
            return std::nullopt;
        }

        quxlang::type_symbol runtime_start = quxlang::subsymbol{
            .of = quxlang::absolute_module_reference{.module_name = "RUNTIME"},
            .name = "PROGRAM_START",
        };
        quxlang::ast2_symboid const symboid = graph.make_request< quxlang::symboid_query >(runtime_start);
        if (symboid.type_is< std::monostate >())
        {
            return std::nullopt;
        }
        if (!symboid.type_is< quxlang::ast2_asm_procedure_declaration >())
        {
            throw quxlang::semantic_compilation_error("RUNTIME::PROGRAM_START must be an ASM_PROCEDURE");
        }

        return runtime_start;
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
    auto write_llvm_text_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_llvm_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const symbol_comment = "; Qux symbol: " + quxlang::to_string(functanoid_symbol) + "\n\n";

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
     * Writes one optimized textual LLVM IR file for qxc output.
     */
    auto write_optimized_llvm_text_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_optimized_llvm_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const symbol_comment = "; Qux symbol: " + quxlang::to_string(functanoid_symbol) + "\n\n";

        std::ofstream outfile(ir_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open optimized LLVM output file: " + ir_path.string());
        }

        outfile.write(symbol_comment.data(), static_cast< std::streamsize >(symbol_comment.size()));
        outfile.write(ir_text.data(), static_cast< std::streamsize >(ir_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write optimized LLVM output file: " + ir_path.string());
        }

        return ir_path;
    }

    /**
     * Writes one LLVM object file for qxc output.
     */
    auto write_object_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::vector< std::byte > const& object_bytes) -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_object_output_path(build_dir, quxlang::mangle(functanoid_symbol));
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
     * Writes one optimized LLVM object file for qxc output.
     */
    auto write_optimized_object_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::vector< std::byte > const& object_bytes)
        -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_optimized_object_output_path(build_dir, quxlang::mangle(functanoid_symbol));
        std::filesystem::create_directories(object_path.parent_path());

        std::ofstream outfile(object_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open optimized LLVM object output file: " + object_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(object_bytes.data()), static_cast< std::streamsize >(object_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write optimized LLVM object output file: " + object_path.string());
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
        std::string const symbol_comment = "# Qux symbol: " + quxlang::to_string(functanoid_symbol) + "\n\n";

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
     * Writes one aggregated output-module LLVM IR file for qxc output.
     */
    auto write_output_module_llvm_text_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        quxlang::type_symbol const& entry_symbol,
        std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_output_module_llvm_output_path(build_dir, output_name);
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const header_comment =
            "; Qux output module: " + output_name + "\n"
            "; Qux entry: " + quxlang::to_string(entry_symbol) + "\n\n";

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
     * Writes one aggregated optimized output-module LLVM IR file for qxc output.
     */
    auto write_optimized_output_module_llvm_text_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        quxlang::type_symbol const& entry_symbol,
        std::string const& ir_text) -> std::filesystem::path
    {
        std::filesystem::path const ir_path = quxlang::qxc_detail::make_optimized_output_module_llvm_output_path(build_dir, output_name);
        std::filesystem::create_directories(ir_path.parent_path());
        std::string const header_comment =
            "; Qux output module: " + output_name + "\n"
            "; Qux entry: " + quxlang::to_string(entry_symbol) + "\n\n";

        std::ofstream outfile(ir_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open optimized output-module LLVM file: " + ir_path.string());
        }

        outfile.write(header_comment.data(), static_cast< std::streamsize >(header_comment.size()));
        outfile.write(ir_text.data(), static_cast< std::streamsize >(ir_text.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write optimized output-module LLVM file: " + ir_path.string());
        }

        return ir_path;
    }

    /**
     * Writes one aggregated output-module LLVM object file for qxc output.
     */
    auto write_output_module_object_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        std::vector< std::byte > const& object_bytes) -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_output_module_object_output_path(build_dir, output_name);
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
     * Writes one aggregated output executable ELF file for qxc output.
     */
    auto write_output_executable_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        std::vector< std::byte > const& file_bytes) -> std::filesystem::path
    {
        std::filesystem::path const executable_path = quxlang::qxc_detail::make_output_executable_output_path(build_dir, output_name);
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
     * Writes one aggregated optimized output executable ELF file for qxc output.
     */
    auto write_optimized_output_executable_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        std::vector< std::byte > const& file_bytes) -> std::filesystem::path
    {
        std::filesystem::path const executable_path = quxlang::qxc_detail::make_optimized_output_executable_output_path(build_dir, output_name);
        std::filesystem::create_directories(executable_path.parent_path());

        std::ofstream outfile(executable_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open optimized output executable file: " + executable_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(file_bytes.data()), static_cast< std::streamsize >(file_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write optimized output executable file: " + executable_path.string());
        }

        std::filesystem::permissions(
            executable_path,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec | std::filesystem::perms::group_read |
                std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec,
            std::filesystem::perm_options::replace);

        return executable_path;
    }

    /**
     * Writes one aggregated optimized output-module LLVM object file for qxc output.
     */
    auto write_optimized_output_module_object_file(
        std::filesystem::path const& build_dir,
        std::string const& output_name,
        std::vector< std::byte > const& object_bytes) -> std::filesystem::path
    {
        std::filesystem::path const object_path = quxlang::qxc_detail::make_optimized_output_module_object_output_path(build_dir, output_name);
        std::filesystem::create_directories(object_path.parent_path());

        std::ofstream outfile(object_path, std::ios::binary | std::ios::trunc);
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to open optimized output-module LLVM object file: " + object_path.string());
        }

        outfile.write(reinterpret_cast< char const* >(object_bytes.data()), static_cast< std::streamsize >(object_bytes.size()));
        if (!outfile)
        {
            throw quxlang::compilation_error("Failed to write optimized output-module LLVM object file: " + object_path.string());
        }

        return object_path;
    }

    /**
     * Shared LLVM packet support data reused when emitting one routine tree as textual LLVM IR.
     */
    struct llvm_packet_support_data
    {
        std::map< quxlang::type_symbol, std::string > procedure_linksymbols;
        std::map< quxlang::type_symbol, quxlang::antestatal_value > antestatal_constants;
        std::map< quxlang::type_symbol, std::vector< quxlang::interface_slot_key > > interface_slots;
        std::map< quxlang::type_symbol, quxlang::enum_info > enum_infos;
        std::map< quxlang::type_symbol, quxlang::flagset_info > flagset_infos;
        std::map< quxlang::type_symbol, quxlang::class_layout > class_layouts;
        std::map< quxlang::type_symbol, quxlang::type_placement_info > type_placements;
    };

    /**
     * One fully discovered routine tree rooted at a single emitted functanoid.
     */
    struct collected_routine_tree
    {
        std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 > routines;
        std::map< quxlang::type_symbol, quxlang::asm_procedure > asm_routines;
        quxlang::qxc_detail::llvm_inlining_dependency_graph dependency_graph;
        llvm_packet_support_data support;
    };

    /**
     * Collects the type and readonly-global inputs needed to lower one routine tree to LLVM IR.
     */
    auto build_llvm_packet_support_data(
        quxlang::compiler_querygraph& graph,
        quxlang::machine_target_info const& machine,
        std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 > const& routines) -> llvm_packet_support_data
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
            std::set< quxlang::type_symbol > const placement_roots = quxlang::vmir2::directly_required_type_placements(routine);
            for (quxlang::type_symbol const& placement_root : placement_roots)
            {
                enqueue_type(placement_root);
            }

            std::set< quxlang::type_symbol > const antestatal_roots = quxlang::vmir2::directly_referenced_antestatal_globals(routine);
            for (quxlang::type_symbol const& antestatal_root : antestatal_roots)
            {
                enqueue_antestatal_global(antestatal_root);
            }

            for (std::pair< quxlang::static_snapshot_ref const, quxlang::vmir2::localdata_entry > const& snapshot_entry : routine.static_snapshots)
            {
                quxlang::type_symbol const snapshot_symbol = quxlang::type_symbol(snapshot_entry.first);
                result.antestatal_constants[snapshot_symbol] = snapshot_entry.second.value;
                enqueue_type(snapshot_entry.second.type);
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

            result.antestatal_constants[symbol] = graph.make_request< quxlang::antestatal_static_value_query >(symbol);
            enqueue_type(graph.make_request< quxlang::variable_type_query >(symbol));
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
                result.type_placements[type] = quxlang::type_placement_info{
                    .size = machine.pointer_size_bytes(),
                    .alignment = machine.pointer_align(),
                };
            }
            else if (!skip_type_placement_query)
            {
                result.type_placements[type] = graph.make_request< quxlang::type_placement_info_query >(type);
            }

            if (!(type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >()))
            {
                continue;
            }

            quxlang::symbol_kind const kind = graph.make_request< quxlang::symbol_type_query >(type);
            if (kind == quxlang::symbol_kind::interface_)
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
            if (kind == quxlang::symbol_kind::enum_)
            {
                result.enum_infos[type] = graph.make_request< quxlang::enum_info_query >(type);
                continue;
            }
            if (kind == quxlang::symbol_kind::flagset_)
            {
                result.flagset_infos[type] = graph.make_request< quxlang::flagset_info_query >(type);
                continue;
            }

            quxlang::class_layout const layout = graph.make_request< quxlang::class_layout_query >(type);
            result.class_layouts[type] = layout;
            for (quxlang::class_field_info const& field : layout.fields)
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
            throw quxlang::compilation_error("Usage: qxc <input directory> <output directory> [targets,...]");
        }

        std::filesystem::path input = argv[1];
        std::filesystem::path output = argv[2];

        std::optional< std::set< std::string > > configured_targets = parse_target_filters(argc, argv);
        input_srcs.emplace(quxlang::load_bundle_sources_for_targets(input, configured_targets));
        file_index.emplace(make_source_file_index(*input_srcs));
        source_index.emplace(*file_index, *input_srcs);

        for (auto const& [target_name, target_config] : input_srcs->targets)
        {
            active_target_name = target_name;
            if (verbose)
            {
                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    std::cout << "Compiling Target: " << target_name << std::endl;
                }
            }

            quxlang::compiler_querygraph graph(*input_srcs, target_name, target_config.target_output_config);

            std::filesystem::path const build_dir = output / target_name / "build";
            std::filesystem::path const output_dir = output / target_name / "output";
            std::filesystem::create_directories(build_dir);
            std::filesystem::create_directories(output_dir);

            struct output_entry
            {
                std::string output_name;
                std::string module_name;
                std::string main_functanoid;
                quxlang::output_kind type;
            };

            std::vector< output_entry > outputs_to_compile;
            if (!target_config.outputs.has_value())
            {
                outputs_to_compile.push_back(output_entry{
                    .output_name = "default",
                    .module_name = "main",
                    .main_functanoid = "::main#()",
                    .type = quxlang::output_kind::executable,
                });
            }
            else
            {
                for (auto const& [output_name, output_config] : *target_config.outputs)
                {
                    outputs_to_compile.push_back(output_entry{
                        .output_name = output_name,
                        .module_name = output_config.module.value_or("main"),
                        .main_functanoid = output_config.main_functanoid.value_or("::main#()"),
                        .type = output_config.type,
                    });
                }
            }

            std::set< quxlang::type_symbol > compiled_vmir2_routines;
            std::set< quxlang::type_symbol > compiled_llvm_routines;
            std::set< quxlang::type_symbol > compiled_asm_routines;
            quxlang::llvm::llvm_backend llvm_backend;
            auto collect_routine_tree =
                [&](quxlang::type_symbol root_symbol,
                    quxlang::vmir2::functanoid_routine3 const& root_routine,
                    std::vector< quxlang::trace_frame > root_traceback,
                    std::optional< quxlang::type_symbol > runtime_program_start) -> collected_routine_tree
            {
                std::vector< std::pair< quxlang::instanciation_reference, std::vector< quxlang::trace_frame > > > pending_functanoids;
                std::set< quxlang::type_symbol > queued_functanoids;
                std::map< quxlang::type_symbol, quxlang::vmir2::functanoid_routine3 > tree_routines;
                std::map< quxlang::type_symbol, quxlang::asm_procedure > asm_routines;
                quxlang::qxc_detail::llvm_inlining_dependency_graph dependency_graph;
                tree_routines.emplace(root_symbol, root_routine);

                auto enqueue_functanoid = [&](quxlang::instanciation_reference const& functanoid, std::vector< quxlang::trace_frame > traceback) -> void
                {
                    quxlang::type_symbol functanoid_symbol = functanoid;
                    if (!queued_functanoids.insert(functanoid_symbol).second)
                    {
                        return;
                    }
                    pending_functanoids.push_back(std::make_pair(functanoid, std::move(traceback)));
                };

                auto collect_asm_routine =
                    [&](quxlang::type_symbol const& asm_symbol,
                        quxlang::ast2_asm_procedure_declaration const& declaration,
                        std::vector< quxlang::trace_frame > dependency_traceback) -> void
                {
                    if (asm_routines.contains(asm_symbol))
                    {
                        return;
                    }

                    quxlang::asm_procedure procedure = graph.make_request< quxlang::asm_procedure_from_symbol_query >(asm_symbol);
                    asm_routines.emplace(asm_symbol, std::move(procedure));

                    functanoid_reference_locations const referenced_functanoids = directly_referenced_functanoid_locations(graph, declaration);
                    for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : referenced_functanoids)
                    {
                        quxlang::type_symbol const& referenced_symbol = referenced_functanoid.first;
                        if (!referenced_symbol.type_is< quxlang::instanciation_reference >())
                        {
                            throw quxlang::compiler_bug("ASM dependency scan returned a non-instanciation reference: " + quxlang::to_string(referenced_symbol));
                        }
                        enqueue_functanoid(
                            referenced_symbol.as< quxlang::instanciation_reference >(),
                            make_dependency_traceback(dependency_traceback, asm_symbol, referenced_functanoid.second));
                    }
                    for (std::pair< quxlang::type_symbol const, std::optional< quxlang::source_location > > const& referenced_functanoid : referenced_functanoids)
                    {
                        dependency_graph[asm_symbol].insert(referenced_functanoid.first);
                    }
                };

                functanoid_reference_locations const root_dependencies = directly_referenced_functanoid_locations(graph, root_routine);
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

                if (runtime_program_start.has_value())
                {
                    quxlang::ast2_symboid const symboid = graph.make_request< quxlang::symboid_query >(*runtime_program_start);
                    if (!symboid.type_is< quxlang::ast2_asm_procedure_declaration >())
                    {
                        throw quxlang::compiler_bug("Runtime PROGRAM_START was not an ASM_PROCEDURE after qxc resolved it");
                    }
                    collect_asm_routine(*runtime_program_start, symboid.get_as< quxlang::ast2_asm_procedure_declaration >(), {});
                }

                while (!pending_functanoids.empty())
                {
                    std::pair< quxlang::instanciation_reference, std::vector< quxlang::trace_frame > > pending_functanoid = std::move(pending_functanoids.back());
                    pending_functanoids.pop_back();

                    quxlang::instanciation_reference functanoid = std::move(pending_functanoid.first);
                    std::vector< quxlang::trace_frame > dependency_traceback = std::move(pending_functanoid.second);
                    quxlang::type_symbol functanoid_symbol = functanoid;
                    if (tree_routines.contains(functanoid_symbol) || asm_routines.contains(functanoid_symbol))
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
                        collect_asm_routine(functanoid_symbol, symboid.get_as< quxlang::ast2_asm_procedure_declaration >(), std::move(dependency_traceback));
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

                    functanoid_reference_locations const referenced_functanoids = directly_referenced_functanoid_locations(graph, procedure);
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
                }

                collected_routine_tree result;
                result.routines = std::move(tree_routines);
                result.asm_routines = std::move(asm_routines);
                result.dependency_graph = std::move(dependency_graph);
                result.support = build_llvm_packet_support_data(graph, target_config.target_output_config, result.routines);
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
                return result;
            };

            auto emit_routine_tree =
                [&](collected_routine_tree const& tree) -> void
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
                    compilable_unit.machine_target.optimization = quxlang::llvm_backend::optimization_level::debug;
                    compilable_unit.source_index = rpnx::cow< quxlang::vmir2::source_index >(*source_index);
                    compilable_unit.procedure_linksymbols = tree.support.procedure_linksymbols;
                    compilable_unit.antestatal_constants = tree.support.antestatal_constants;
                    compilable_unit.interface_slots = tree.support.interface_slots;
                    compilable_unit.enum_infos = tree.support.enum_infos;
                    compilable_unit.flagset_infos = tree.support.flagset_infos;
                    compilable_unit.class_layouts = tree.support.class_layouts;
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
                        std::map< quxlang::type_symbol, quxlang::asm_procedure >::const_iterator helper_iter = tree.asm_routines.find(helper_symbol);
                        if (helper_iter == tree.asm_routines.end())
                        {
                            continue;
                        }
                        compilable_unit.asm_functions.insert(*helper_iter);
                    }

                    quxlang::llvm_backend::llvm_compiled_unit const llvm_unit = llvm_backend.compile(compilable_unit);
                    std::filesystem::path const llvm_path = write_llvm_text_file(build_dir, routine_symbol, llvm_unit.llvm_ir_text);
                    std::filesystem::path const optimized_llvm_path = write_optimized_llvm_text_file(build_dir, routine_symbol, llvm_unit.optimized_llvm_ir_text);
                    std::filesystem::path const object_path = write_object_file(build_dir, routine_symbol, llvm_unit.object_file);
                    std::filesystem::path const optimized_object_path = write_optimized_object_file(build_dir, routine_symbol, llvm_unit.optimized_object_file);
                    if (verbose)
                    {
                        std::cout << "Wrote LLVM: " << quxlang::to_string(routine_symbol) << " -> " << llvm_path.string() << std::endl;
                        std::cout << "Wrote optimized LLVM: " << quxlang::to_string(routine_symbol) << " -> " << optimized_llvm_path.string() << std::endl;
                        std::cout << "Wrote LLVM object: " << quxlang::to_string(routine_symbol) << " -> " << object_path.string() << std::endl;
                        std::cout << "Wrote optimized LLVM object: " << quxlang::to_string(routine_symbol) << " -> " << optimized_object_path.string() << std::endl;
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
                                .optimization = quxlang::llvm_backend::optimization_level::debug,
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
                    if (!sym.type_is< quxlang::ast2_static_test >())
                    {
                        throw quxlang::compiler_bug("list_static_tests returned a non-static-test symbol: " + quxlang::to_string(static_test_symbol));
                    }

                    quxlang::ast2_static_test const& static_test_decl = sym.get_as< quxlang::ast2_static_test >();
                    if (static_test_decl.expected_mode == quxlang::static_test_expected_mode::expect_compilation_failure)
                    {
                        continue;
                    }

                    if (verbose)
                    {
                        std::cout << "Compiling STATIC_TEST VMIR2: " << quxlang::to_string(static_test_symbol) << std::endl;
                    }

                    quxlang::vmir2::functanoid_routine3 static_test_routine = graph.make_request< quxlang::static_test_vmir_query >(static_test_symbol);
                    emit_routine_tree(collect_routine_tree(static_test_symbol, static_test_routine, {}, std::nullopt));
                }
            }

            std::optional< quxlang::type_symbol > const runtime_program_start = try_resolve_runtime_program_start(graph, target_config);

            for (output_entry const& output_entry : outputs_to_compile)
            {
                if (!target_config.module_configurations.contains(output_entry.module_name))
                {
                    throw quxlang::semantic_compilation_error("Target '" + target_name + "' output '" + output_entry.output_name + "' references unknown module '" + output_entry.module_name + "'");
                }

                if (verbose)
                {
                    std::cout << "Compiling VMIR2 output: " << target_name << "/" << output_entry.output_name << std::endl;
                }

                quxlang::instanciation_reference entry_functanoid = resolve_entry_functanoid(graph, output_entry.module_name, output_entry.main_functanoid);
                quxlang::vmir2::functanoid_routine3 entry_routine = graph.make_request< quxlang::vm_procedure3_query >(entry_functanoid);
                collected_routine_tree const output_tree = collect_routine_tree(entry_functanoid, entry_routine, {}, runtime_program_start);
                emit_routine_tree(output_tree);

                quxlang::llvm_backend::llvm_compilable_unit output_module_unit;
                output_module_unit.target_name = entry_functanoid;
                output_module_unit.target_code = entry_routine;
                output_module_unit.machine_target.machine = target_config.target_output_config;
                output_module_unit.machine_target.optimization = quxlang::llvm_backend::optimization_level::debug;
                output_module_unit.whole_module = true;
                output_module_unit.whole_module_output_kind = output_entry.type;
                std::optional< std::string > executable_entry_symbol;
                if (runtime_program_start.has_value() && output_entry.type == quxlang::output_kind::executable && target_config.target_output_config.os_type == quxlang::os::linux &&
                    target_config.target_output_config.binary_type == quxlang::binary::elf)
                {
                    executable_entry_symbol = quxlang::mangle(*runtime_program_start);
                    output_module_unit.executable_entry_symbol = executable_entry_symbol;
                }
                output_module_unit.source_index = rpnx::cow< quxlang::vmir2::source_index >(*source_index);
                output_module_unit.procedure_linksymbols = output_tree.support.procedure_linksymbols;
                output_module_unit.antestatal_constants = output_tree.support.antestatal_constants;
                output_module_unit.interface_slots = output_tree.support.interface_slots;
                output_module_unit.enum_infos = output_tree.support.enum_infos;
                output_module_unit.flagset_infos = output_tree.support.flagset_infos;
                output_module_unit.class_layouts = output_tree.support.class_layouts;
                output_module_unit.type_placements = output_tree.support.type_placements;
                for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& routine_entry : output_tree.routines)
                {
                    if (routine_entry.first == quxlang::type_symbol(entry_functanoid))
                    {
                        continue;
                    }
                    output_module_unit.inlinable_functions.insert(routine_entry);
                }
                output_module_unit.asm_functions = output_tree.asm_routines;

                quxlang::llvm_backend::llvm_compiled_unit const output_module = llvm_backend.compile(output_module_unit);
                std::filesystem::path const output_module_path =
                    write_output_module_llvm_text_file(build_dir, output_entry.output_name, entry_functanoid, output_module.llvm_ir_text);
                std::filesystem::path const optimized_output_module_path =
                    write_optimized_output_module_llvm_text_file(build_dir, output_entry.output_name, entry_functanoid, output_module.optimized_llvm_ir_text);
                std::filesystem::path const output_module_object_path =
                    write_output_module_object_file(build_dir, output_entry.output_name, output_module.object_file);
                std::filesystem::path const optimized_output_module_object_path =
                    write_optimized_output_module_object_file(build_dir, output_entry.output_name, output_module.optimized_object_file);
                if (verbose)
                {
                    std::cout << "Wrote output-module LLVM: " << target_name << "/" << output_entry.output_name << " -> " << output_module_path.string() << std::endl;
                    std::cout << "Wrote optimized output-module LLVM: " << target_name << "/" << output_entry.output_name << " -> " << optimized_output_module_path.string() << std::endl;
                    std::cout << "Wrote output-module LLVM object: " << target_name << "/" << output_entry.output_name << " -> " << output_module_object_path.string() << std::endl;
                    std::cout << "Wrote optimized output-module LLVM object: " << target_name << "/" << output_entry.output_name << " -> " << optimized_output_module_object_path.string() << std::endl;
                }

                if (output_entry.type == quxlang::output_kind::executable && target_config.target_output_config.os_type == quxlang::os::linux &&
                    target_config.target_output_config.binary_type == quxlang::binary::elf)
                {
                    quxlang::elf_linker linker;
                    std::map< std::string, std::string > symbol_display_names;
                    auto add_symbol_display_name = [&symbol_display_names](quxlang::type_symbol const& symbol)
                    {
                        symbol_display_names.emplace(quxlang::mangle(symbol), quxlang::to_string(symbol));
                    };
                    add_symbol_display_name(entry_functanoid);
                    for (std::pair< quxlang::type_symbol const, quxlang::vmir2::functanoid_routine3 > const& routine_entry : output_tree.routines)
                    {
                        add_symbol_display_name(routine_entry.first);
                    }
                    for (std::pair< quxlang::type_symbol const, quxlang::asm_procedure > const& asm_entry : output_tree.asm_routines)
                    {
                        add_symbol_display_name(asm_entry.first);
                    }
                    for (std::pair< quxlang::type_symbol const, quxlang::antestatal_value > const& constant_entry : output_tree.support.antestatal_constants)
                    {
                        add_symbol_display_name(constant_entry.first);
                    }
                    quxlang::elf_link_options const debug_link_options{
                        .preserve_symbols = true,
                        .symbol_display_names = std::move(symbol_display_names),
                    };
                    std::string const entry_symbol = executable_entry_symbol.value_or("_start");
                    std::vector< std::byte > const executable_bytes =
                        linker.link_linux_executable(target_config.target_output_config, output_module.object_file, entry_symbol, debug_link_options);
                    std::vector< std::byte > const optimized_executable_bytes =
                        linker.link_linux_executable(target_config.target_output_config, output_module.optimized_object_file, entry_symbol);
                    std::filesystem::path const executable_path =
                        write_output_executable_file(output_dir, output_entry.output_name, executable_bytes);
                    std::filesystem::path const optimized_executable_path =
                        write_optimized_output_executable_file(output_dir, output_entry.output_name, optimized_executable_bytes);
                    if (verbose)
                    {
                        std::cout << "Wrote output executable: " << target_name << "/" << output_entry.output_name << " -> " << executable_path.string() << std::endl;
                        std::cout << "Wrote optimized output executable: " << target_name << "/" << output_entry.output_name << " -> " << optimized_executable_path.string() << std::endl;
                    }
                }
            }
            active_target_name.reset();
        }

        return 0;
    }
    catch (quxlang::compilation_error const& error)
    {
        print_compilation_error(std::cerr, error, source_index.has_value() ? &*source_index : nullptr, active_target_name);
        return 1;
    }
}
