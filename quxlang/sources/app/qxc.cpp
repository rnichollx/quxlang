// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include "quxlang/data/contextual_type_reference.hpp"
#include "quxlang/compiler_querygraph.hpp"
#include "quxlang/manipulators/mangler.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"
#include "quxlang/queries/functanoid_indirectly_instantiated_functanoids.hpp"
#include "quxlang/queries/instanciation.hpp"
#include "quxlang/queries/lookup.hpp"
#include "quxlang/queries/vm_procedure3.hpp"
#include "quxlang/source_loader.hpp"
#include "quxlang/vmir2/assembler.hpp"
#include "quxlang/vmir2/source_index.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
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
    auto concrete_functanoid_from_symbol(quxlang::type_symbol const& symbol) -> std::optional< quxlang::instanciation_reference >
    {
        if (symbol.type_is< quxlang::instanciation_reference >())
        {
            return symbol.as< quxlang::instanciation_reference >();
        }

        if (symbol.type_is< quxlang::temploid_reference >())
        {
            quxlang::temploid_reference const& selected_function = symbol.as< quxlang::temploid_reference >();
            if (overload_has_unspecialized_parameters(selected_function.which))
            {
                throw quxlang::semantic_compilation_error("Cannot emit uninstantiated procedure pointer target: " + quxlang::to_string(symbol));
            }

            return quxlang::instanciation_reference{
                .temploid = selected_function,
                .params = instantiate_declared_overload(selected_function.which),
            };
        }

        return std::nullopt;
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

        if (auto concrete = concrete_functanoid_from_symbol(*resolved_entry); concrete.has_value())
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
     * Writes one VMIR2 routine text file for qxc output.
     */
    void write_vmir2_text_file(std::filesystem::path const& build_dir, quxlang::type_symbol const& functanoid_symbol, std::string const& ir_text)
    {
        std::filesystem::path const ir_path = build_dir / (quxlang::mangle(functanoid_symbol) + ".vmir2");

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
    }

    /**
     * Formats a traceback location using source-bundle-relative paths.
     */
    auto format_traceback_location(quxlang::vmir2::source_index const* source_index, std::optional< quxlang::source_location > const& location) -> std::string
    {
        if (!location.has_value())
        {
            return {};
        }

        std::string formatted;
        if (source_index != nullptr)
        {
            formatted = source_index->format(location);
        }
        else
        {
            formatted = quxlang::source_location_suffix(location);
        }

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
     * Prints a qxc compilation error with the source traceback captured by the compiler.
     */
    void print_compilation_error(std::ostream& output, quxlang::compilation_error const& error, quxlang::vmir2::source_index const* source_index)
    {
        output << "qxc: compilation error: " << error.what() << '\n';
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
            if (verbose)
            {
                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    std::cout << "Compiling Target: " << target_name << std::endl;
                }
            }

            quxlang::compiler_querygraph graph(*input_srcs, target_name, target_config.target_output_config);

            std::filesystem::path const build_dir = output / target_name / "build";
            std::filesystem::create_directories(build_dir);

            struct output_entry
            {
                std::string output_name;
                std::string module_name;
                std::string main_functanoid;
            };

            std::vector< output_entry > outputs_to_compile;
            if (target_config.outputs.empty())
            {
                outputs_to_compile.push_back(output_entry{
                    .output_name = "default",
                    .module_name = "main",
                    .main_functanoid = "::main#()",
                });
            }
            else
            {
                for (auto const& [output_name, output_config] : target_config.outputs)
                {
                    outputs_to_compile.push_back(output_entry{
                        .output_name = output_name,
                        .module_name = output_config.module.value_or("main"),
                        .main_functanoid = output_config.main_functanoid.value_or("::main#()"),
                    });
                }
            }

            std::set< quxlang::type_symbol > compiled_functanoids;
            for (output_entry const& output_entry : outputs_to_compile)
            {
                if (!target_config.module_configurations.contains(output_entry.module_name))
                {
                    throw quxlang::semantic_compilation_error("Target '" + target_name + "' output '" + output_entry.output_name + "' references unknown module '" + output_entry.module_name + "'");
                }

                if (verbose)
                {
                    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                    {
                        std::cout << "Compiling VMIR2 output: " << target_name << "/" << output_entry.output_name << std::endl;
                    }
                }

                std::vector< quxlang::instanciation_reference > pending_functanoids;
                std::set< quxlang::type_symbol > queued_functanoids;

                auto enqueue_functanoid = [&](quxlang::instanciation_reference const& functanoid) -> void
                {
                    quxlang::type_symbol functanoid_symbol = functanoid;
                    if (compiled_functanoids.contains(functanoid_symbol) || !queued_functanoids.insert(functanoid_symbol).second)
                    {
                        return;
                    }
                    pending_functanoids.push_back(functanoid);
                };

                enqueue_functanoid(resolve_entry_functanoid(graph, output_entry.module_name, output_entry.main_functanoid));

                while (!pending_functanoids.empty())
                {
                    quxlang::instanciation_reference functanoid = std::move(pending_functanoids.back());
                    pending_functanoids.pop_back();

                    quxlang::type_symbol functanoid_symbol = functanoid;
                    if (compiled_functanoids.contains(functanoid_symbol))
                    {
                        continue;
                    }

                    if (verbose)
                    {
                        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                        {
                            std::cout << "Compiling VMIR2 function: " << quxlang::to_string(functanoid_symbol) << std::endl;
                        }
                    }

                    quxlang::vmir2::functanoid_routine3 const procedure = graph.make_request< quxlang::vm_procedure3_query >(functanoid);
                    quxlang::vmir2::assembler assembler(procedure, *source_index);
                    std::string const ir_text = assembler.to_string(procedure);

                    write_vmir2_text_file(build_dir, functanoid_symbol, ir_text);
                    compiled_functanoids.insert(functanoid_symbol);

                    std::set< quxlang::type_symbol > const referenced_functanoids = graph.make_request< quxlang::functanoid_indirectly_instantiated_functanoids_query >(
                        quxlang::functanoid_requirement_input{.functanoid = functanoid, .compilation_type = quxlang::functanoid_compilation_type::all});
                    for (quxlang::type_symbol const& referenced_symbol : referenced_functanoids)
                    {
                        if (!referenced_symbol.type_is< quxlang::instanciation_reference >())
                        {
                            throw quxlang::compiler_bug("functanoid_indirectly_instantiated_functanoids returned a non-instanciation reference: " + quxlang::to_string(referenced_symbol));
                        }
                        enqueue_functanoid(referenced_symbol.as< quxlang::instanciation_reference >());
                    }
                }
            }
        }

        return 0;
    }
    catch (quxlang::compilation_error const& error)
    {
        print_compilation_error(std::cerr, error, source_index.has_value() ? &*source_index : nullptr);
        return 1;
    }
}
