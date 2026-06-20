// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/linker/elf_linker.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/output_llvm_input.hpp>
#include <quxlang/queries/specs/llvm_output_binary_artifact_spec.hpp>

#include <map>

rpnx::querygraph::coroutine< quxlang::llvm_output_binary_artifact_spec > quxlang::llvm_output_binary_artifact_impl(std::string input)
{
    output_query_output const output_info = co_await rpnx::querygraph::request< output_binary_information_query >(input);
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    backend_llvm_options const llvm_options = co_await rpnx::querygraph::request< output_llvm_backend_options_query >(input);
    llvm_backend::llvm_compilable_unit const llvm_input = co_await rpnx::querygraph::request< output_llvm_input_query >(input);
    llvm_backend::llvm_compiled_unit const compiled = co_await rpnx::querygraph::request< llvm_compiled_output_query >(input);

    if (output_info.type == output_kind::executable && target_config.target_output_config.os_type == os::linux && target_config.target_output_config.binary_type == binary::elf)
    {
        std::vector< std::byte > const& object_file =
            llvm_options.mode == backend_llvm_mode::debug ? compiled.object_file : compiled.optimized_object_file;
        std::string const entry_symbol = llvm_input.executable_entry_symbol.value_or("_start");
        auto output_symbol_display_names = [](llvm_backend::llvm_compilable_unit const& llvm_input) -> std::map< std::string, std::string >
        {
            std::map< std::string, std::string > result;
            auto add_symbol_display_name = [&result](type_symbol const& symbol)
            {
                result.emplace(to_string(symbol), to_string(symbol));
            };

            add_symbol_display_name(llvm_input.target_name);
            for (std::pair< type_symbol const, vmir2::functanoid_routine3 > const& routine_entry : llvm_input.inlinable_functions)
            {
                add_symbol_display_name(routine_entry.first);
            }
            for (std::pair< type_symbol const, asm_procedure > const& asm_entry : llvm_input.asm_functions)
            {
                add_symbol_display_name(asm_entry.first);
            }
            for (std::pair< type_symbol const, antestatal_value > const& constant_entry : llvm_input.antestatal_constants)
            {
                add_symbol_display_name(constant_entry.first);
            }
            for (std::pair< type_symbol const, type_symbol > const& object_entry : llvm_input.object_reference_types)
            {
                add_symbol_display_name(object_entry.first);
            }

            return result;
        };
        elf_link_options const link_options{
            .preserve_symbols = llvm_options.mode == backend_llvm_mode::debug,
            .symbol_display_names = llvm_options.mode == backend_llvm_mode::debug ? output_symbol_display_names(llvm_input) : std::map< std::string, std::string >{},
        };

        elf_linker linker;
        co_return linker.link_linux_executable(target_config.target_output_config, object_file, entry_symbol, link_options);
    }

    throw quxlang::semantic_compilation_error("LLVM output kind is not supported for output '" + input + "'");
}
