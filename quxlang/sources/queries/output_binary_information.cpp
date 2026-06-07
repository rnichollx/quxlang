// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/output_binary_information_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_binary_information_spec > quxlang::output_binary_information_impl(std::string input)
{
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});

    if (!target_config.outputs.has_value())
    {
        if (input != "default")
        {
            throw quxlang::semantic_compilation_error("Unknown output '" + input + "'");
        }

        co_return output_query_output{
            .output_name = "default",
            .module_name = "main",
            .main_functanoid = "::main#()",
            .type = output_kind::executable,
        };
    }

    std::map< std::string, output_config >::const_iterator output_iter = target_config.outputs->find(input);
    if (output_iter == target_config.outputs->end())
    {
        throw quxlang::semantic_compilation_error("Unknown output '" + input + "'");
    }

    output_config const& config = output_iter->second;
    co_return output_query_output{
        .output_name = input,
        .module_name = config.module.value_or("main"),
        .main_functanoid = config.main_functanoid.value_or("::main#()"),
        .type = config.type,
    };
}
