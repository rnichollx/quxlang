// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/queries/specs/output_binary_information_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_binary_information_spec > quxlang::output_binary_information_impl(std::string input)
{
    /** Parses an output entry symbol and rejects trailing input. */
    auto parse_entry_symbol = [](std::string const& text) -> type_symbol
    {
        parsers::parsing_context context = parsers::make_unlocated_parsing_context(text);
        type_symbol result = parsers::parse_type_symbol(context);
        if (context.iter_pos != context.iter_end)
        {
            throw syntax_compilation_error("Input not fully parsed");
        }
        return result;
    };

    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});

    source_bundle const& bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    std::string target = co_await rpnx::querygraph::request< configured_target_query >(std::monostate{});
    std::map< std::string, output_config >::const_iterator output_iter = bundle.outputs.find(input);
    if (output_iter == bundle.outputs.end() || output_iter->second.target != target)
    {
        throw semantic_compilation_error("Unknown output '" + input + "' for target '" + target + "'");
    }

    output_config const& config = output_iter->second;
    std::optional< type_symbol > main_functanoid = std::nullopt;
    if (config.type != output_kind::unit_test_suite)
    {
        main_functanoid = parse_entry_symbol(config.main_functanoid.value_or("::main#()"));
    }
    else if (config.main_functanoid.has_value())
    {
        throw semantic_compilation_error("Output '" + input + "' of type unit_test_suite cannot configure main_functanoid");
    }

    std::vector< std::string > module_names;
    if (config.type == output_kind::unit_test_suite)
    {
        if (config.main_module.has_value())
        {
            throw semantic_compilation_error("Output '" + input + "' of type unit_test_suite cannot configure main_module");
        }
        module_names = config.test_modules.value_or(std::vector< std::string >{"main"});
    }
    else
    {
        if (config.test_modules.has_value())
        {
            throw semantic_compilation_error("Output '" + input + "' can configure test_modules only when its type is unit_test_suite");
        }
        if (config.main_module.has_value() && config.type != output_kind::executable)
        {
            throw semantic_compilation_error("Output '" + input + "' can configure main_module only when its type is executable");
        }
        module_names.push_back(config.main_module.value_or("main"));
    }

    if (module_names.empty())
    {
        throw semantic_compilation_error("Output '" + input + "' must select at least one module");
    }
    for (std::string const& module_name : module_names)
    {
        if (!target_config.module_configurations.contains(module_name))
        {
            throw semantic_compilation_error("Output '" + input + "' references unknown module '" + module_name + "'");
        }
    }

    co_return output_query_output{
        .output_name = input,
        .module_names = std::move(module_names),
        .main_functanoid = main_functanoid,
        .type = config.type,
    };
}
