// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/linker/elf_linker.hpp>
#include <quxlang/linker/macho_linker.hpp>
#include <quxlang/linker/pe_linker.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/output_llvm_input.hpp>
#include <quxlang/queries/specs/llvm_output_binary_artifact_spec.hpp>

#include <map>
#include <set>
#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::llvm_output_binary_artifact_spec > quxlang::llvm_output_binary_artifact_impl(std::string input)
{
    output_query_output const& output_info = co_await rpnx::querygraph::request< output_binary_information_query >(input);
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    backend_llvm_options const& llvm_options = co_await rpnx::querygraph::request< output_llvm_backend_options_query >(input);
    llvm_component_catalog const& early_init_input = co_await rpnx::querygraph::request< output_llvm_catalog_query >({input, llvm_output_component::early_init});
    std::vector< llvm_output_query_input > const& identities = co_await rpnx::querygraph::request< llvm_compilation_unit_identities_query >(input);
    std::vector< rpnx::querygraph::request< llvm_post_codegen_query > > codegen_requests;
    codegen_requests.reserve(identities.size());
    for (llvm_output_query_input const& identity : identities)
    {
        codegen_requests.emplace_back(identity);
        co_yield rpnx::querygraph::dependency(codegen_requests.back());
    }
    if (identities.empty())
    {
        throw compiler_bug("LLVM compilation produced no native linker inputs");
    }
    std::vector< llvm_component_catalog const* > llvm_inputs;
    std::vector< llvm_output_query_input > const& components = co_await rpnx::querygraph::request< llvm_output_component_identities_query >(input);
    std::set< llvm_output_component > seen_components;
    for (llvm_output_query_input const& component : components)
    {
        if (seen_components.insert(component.component).second)
        {
            llvm_inputs.push_back(&(co_await rpnx::querygraph::request< output_llvm_catalog_query >({input, component.component})));
        }
    }
    std::vector< std::vector< std::byte > > object_files;
    object_files.reserve(identities.size());
    for (rpnx::querygraph::request< llvm_post_codegen_query >& request : codegen_requests)
    {
        llvm_backend::llvm_post_codegen_unit const& object = co_await request;
        object_files.push_back(object.object_file);
    }
    llvm_backend::llvm_postoptimized_unit const& first_unit = co_await rpnx::querygraph::request< llvm_postoptimize_query >(identities.front());

    if ((output_info.type == output_kind::executable || output_info.type == output_kind::unit_test_suite) && target_config.target_output_config.os_type == os::linux && target_config.target_output_config.binary_type == binary::elf)
    {
        std::string entry_symbol = early_init_input.executable_entry_symbol.value_or("_start");
        auto output_symbol_display_names = [](std::vector< llvm_component_catalog const* > const& llvm_inputs) -> std::map< std::string, std::string >
        {
            std::map< std::string, std::string > result;
            auto add_symbol_display_name = [&result](type_symbol const& symbol)
            {
                result.emplace(to_string(symbol), to_string(symbol));
            };

            for (llvm_component_catalog const* llvm_input : llvm_inputs)
            {
                add_symbol_display_name(llvm_input->target_name);
                for (type_symbol const& routine_entry : llvm_input->inlinable_functions)
                {
                    add_symbol_display_name(routine_entry);
                }
                for (type_symbol const& asm_entry : llvm_input->asm_functions)
                {
                    add_symbol_display_name(asm_entry);
                }
                for (type_symbol const& constant_entry : llvm_input->antestatal_constants)
                {
                    add_symbol_display_name(constant_entry);
                }
                for (std::pair< type_symbol const, type_symbol > const& object_entry : llvm_input->object_reference_types)
                {
                    add_symbol_display_name(object_entry.first);
                }
            }

            return result;
        };
        auto dynamic_imports = [](std::vector< llvm_component_catalog const* > const& llvm_inputs) -> std::vector< elf_dynamic_import >
        {
            std::map< std::string, elf_dynamic_import > imports_by_relocation_symbol;
            for (llvm_component_catalog const* llvm_input : llvm_inputs)
            {
                for (type_symbol const& symbol : llvm_input->extern_procedures)
                {
                    std::map< type_symbol, std::string >::const_iterator link_name = llvm_input->procedure_linksymbols.find(symbol);
                    std::map< type_symbol, std::string >::const_iterator library = llvm_input->extern_procedure_libraries.find(symbol);
                    if (link_name == llvm_input->procedure_linksymbols.end() || library == llvm_input->extern_procedure_libraries.end())
                    {
                        throw compiler_bug("Extern procedure is missing linker metadata: " + to_string(symbol));
                    }

                    std::string version;
                    std::map< type_symbol, std::string >::const_iterator found_version = llvm_input->extern_procedure_versions.find(symbol);
                    if (found_version != llvm_input->extern_procedure_versions.end())
                    {
                        version = found_version->second;
                    }
                    std::string relocation_symbol_name = link_name->second;
                    if (!version.empty())
                    {
                        relocation_symbol_name += "@" + version;
                    }
                    imports_by_relocation_symbol.emplace(
                        relocation_symbol_name,
                        elf_dynamic_import{
                            .relocation_symbol_name = relocation_symbol_name,
                            .symbol_name = link_name->second,
                            .library_name = library->second,
                            .version = std::move(version),
                            .optional = llvm_input->optional_extern_procedures.contains(symbol),
                        });
                }
            }
            std::vector< elf_dynamic_import > result;
            result.reserve(imports_by_relocation_symbol.size());
            for (std::pair< std::string const, elf_dynamic_import >& import : imports_by_relocation_symbol)
            {
                result.push_back(std::move(import.second));
            }
            return result;
        };
        elf_link_options link_options{
            .preserve_debug_information = build_type_has_debug_information(*llvm_options.build_type),
            .preserve_symbols = build_type_has_debug_information(*llvm_options.build_type),
            .source_filename = first_unit.source_filename,
            .symbol_display_names = build_type_has_debug_information(*llvm_options.build_type) ? output_symbol_display_names(llvm_inputs) : std::map< std::string, std::string >{},
            .dynamic_imports = dynamic_imports(llvm_inputs),
        };

        elf_linker linker;
        co_return linker.link_linux_executable(target_config.target_output_config, object_files, entry_symbol, link_options);
    }

    if ((output_info.type == output_kind::executable || output_info.type == output_kind::unit_test_suite) &&
        target_config.target_output_config.os_type == os::macos &&
        target_config.target_output_config.binary_type == binary::macho)
    {
        std::string entry_symbol = early_init_input.executable_entry_symbol.value_or("_start");
        std::map< std::string, macho_dynamic_import > imports_by_relocation_symbol;
        for (llvm_component_catalog const* llvm_input : llvm_inputs)
        {
            for (type_symbol const& symbol : llvm_input->extern_procedures)
            {
                std::map< type_symbol, std::string >::const_iterator link_name =
                    llvm_input->procedure_linksymbols.find(symbol);
                std::map< type_symbol, std::string >::const_iterator library =
                    llvm_input->extern_procedure_libraries.find(symbol);
                if (link_name == llvm_input->procedure_linksymbols.end() ||
                    library == llvm_input->extern_procedure_libraries.end())
                {
                    throw compiler_bug("Extern procedure is missing linker metadata: " + to_string(symbol));
                }
                if (llvm_input->extern_procedure_versions.contains(symbol))
                {
                    throw semantic_compilation_error("Mach-O extern procedures do not support symbol versions: " +
                                                     to_string(symbol));
                }

                macho_dynamic_import import{
                    .relocation_symbol_name = link_name->second,
                    .symbol_name = link_name->second,
                    .library_name = library->second,
                    .optional = llvm_input->optional_extern_procedures.contains(symbol),
                };
                std::pair< std::map< std::string, macho_dynamic_import >::iterator, bool > inserted =
                    imports_by_relocation_symbol.emplace(import.relocation_symbol_name, import);
                if (!inserted.second && (inserted.first->second.symbol_name != import.symbol_name ||
                                         inserted.first->second.library_name != import.library_name ||
                                         inserted.first->second.optional != import.optional))
                {
                    throw semantic_compilation_error("Conflicting Mach-O dynamic import metadata for symbol: " +
                                                     import.relocation_symbol_name);
                }
            }
        }

        std::vector< macho_dynamic_import > imports;
        imports.reserve(imports_by_relocation_symbol.size());
        for (std::pair< std::string const, macho_dynamic_import >& import : imports_by_relocation_symbol)
        {
            imports.push_back(std::move(import.second));
        }
        macho_linker linker;
        co_return linker.link_macos_executable(target_config.target_output_config, object_files, entry_symbol,
                                               macho_link_options{
                                                   .preserve_debug_information = build_type_has_debug_information(*llvm_options.build_type),
                                                   .signature_identifier = input,
                                                   .dynamic_imports = std::move(imports),
                                               });
    }

    if ((output_info.type == output_kind::executable || output_info.type == output_kind::unit_test_suite) &&
        target_config.target_output_config.os_type == os::windows &&
        target_config.target_output_config.binary_type == binary::pe)
    {
        std::string entry_symbol = early_init_input.executable_entry_symbol.value_or("mainCRTStartup");
        std::vector< pe_dynamic_import > imports;
        std::set< std::pair< std::string, std::string > > imported_symbols;
        for (llvm_component_catalog const* llvm_input : llvm_inputs)
        {
            for (type_symbol const& symbol : llvm_input->extern_procedures)
            {
                std::map< type_symbol, std::string >::const_iterator link_name = llvm_input->procedure_linksymbols.find(symbol);
                std::map< type_symbol, std::string >::const_iterator library = llvm_input->extern_procedure_libraries.find(symbol);
                if (link_name == llvm_input->procedure_linksymbols.end() || library == llvm_input->extern_procedure_libraries.end())
                {
                    throw compiler_bug("Extern procedure is missing linker metadata: " + to_string(symbol));
                }
                if (imported_symbols.emplace(link_name->second, library->second).second)
                {
                    imports.push_back(pe_dynamic_import{
                        .symbol_name = link_name->second,
                        .library_name = library->second,
                        .optional = llvm_input->optional_extern_procedures.contains(symbol),
                    });
                }
            }
        }
        pe_linker linker;
        co_return linker.link_windows_executable(target_config.target_output_config, object_files, entry_symbol, pe_link_options{.preserve_debug_information = build_type_has_debug_information(*llvm_options.build_type), .dynamic_imports = std::move(imports)});
    }

    throw quxlang::semantic_compilation_error("LLVM output kind is not supported for output '" + input + "'");
}
