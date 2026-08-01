// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/llvm_output_component_identities_spec.hpp>

#include <utility>
#include <vector>

rpnx::querygraph::coroutine< quxlang::llvm_output_component_identities_spec > quxlang::llvm_output_component_identities_impl(std::string input)
{
    output_query_output const& output_info =
        co_await rpnx::querygraph::request< output_binary_information_query >(input);
    std::vector< cpu_stepping_configuration > const& target_steppings =
        co_await rpnx::querygraph::request< target_steppings_query >(std::monostate{});
    llvm_output_query_input early_init_identity{
        .output_name = input,
        .component = llvm_output_component::early_init,
    };
    llvm_backend::llvm_compilable_unit const& early_init_input =
        co_await rpnx::querygraph::request< output_llvm_input_query >(early_init_identity);

    bool stepped_output =
        output_info.type == output_kind::executable || output_info.type == output_kind::unit_test_suite;
    if (stepped_output && target_steppings.empty())
    {
        throw semantic_compilation_error("Executable LLVM compilation requires at least one target stepping");
    }
    if (stepped_output && target_steppings.size() > 1 && !early_init_input.post_detect_functanoid.has_value())
    {
        throw semantic_compilation_error(
            "Multiple target steppings require MODULE(RUNTIME)::POST_DETECT for runtime stepping selection");
    }
    if (stepped_output && target_steppings.size() > 1 && !early_init_input.executable_entry_symbol.has_value())
    {
        throw semantic_compilation_error(
            "Multiple target steppings require MODULE(RUNTIME)::PROGRAM_START for runtime stepping selection");
    }

    std::vector< llvm_output_query_input > result;
    if (stepped_output)
    {
        result.reserve(
            1 + target_steppings.size() * (early_init_input.post_detect_functanoid.has_value() ? 2 : 1));
    }
    result.push_back(std::move(early_init_identity));
    if (stepped_output)
    {
        for (std::size_t stepping_index = 0; stepping_index < target_steppings.size(); ++stepping_index)
        {
            result.push_back(llvm_output_query_input{
                .output_name = input,
                .stepping_index = stepping_index,
                .component = llvm_output_component::main_program,
            });
            if (early_init_input.post_detect_functanoid.has_value())
            {
                result.push_back(llvm_output_query_input{
                    .output_name = input,
                    .stepping_index = stepping_index,
                    .component = llvm_output_component::post_detect,
                });
            }
        }
    }
    co_return result;
}
