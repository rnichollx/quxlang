// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/queries/specs/output_binary_information_spec.hpp>

namespace
{
    auto parse_type_symbol_text(std::string const& text) -> quxlang::type_symbol
    {
        quxlang::parsers::parsing_context ctx = quxlang::parsers::make_unlocated_parsing_context(text);
        quxlang::type_symbol result = quxlang::parsers::parse_type_symbol(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw quxlang::syntax_compilation_error("Input not fully parsed");
        }
        return result;
    }

    auto default_entry_functanoid() -> quxlang::type_symbol
    {
        return parse_type_symbol_text("::main#()");
    }
} // namespace

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
            .main_functanoid = default_entry_functanoid(),
            .type = output_kind::executable,
        };
    }

    std::map< std::string, output_config >::const_iterator output_iter = target_config.outputs->find(input);
    if (output_iter == target_config.outputs->end())
    {
        throw quxlang::semantic_compilation_error("Unknown output '" + input + "'");
    }

    output_config const& config = output_iter->second;
    std::optional< type_symbol > main_functanoid = std::nullopt;
    if (config.type != output_kind::unit_test_suite)
    {
        main_functanoid = parse_type_symbol_text(config.main_functanoid.value_or("::main#()"));
    }
    else if (config.main_functanoid.has_value())
    {
        throw semantic_compilation_error("Output '" + input + "' of type unit_test_suite cannot configure main_functanoid");
    }

    co_return output_query_output{
        .output_name = input,
        .module_name = config.module.value_or("main"),
        .main_functanoid = main_functanoid,
        .type = config.type,
    };
}
