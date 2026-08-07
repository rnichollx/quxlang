// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/queries/specs/output_binary_information_spec.hpp>

#include "query_helpers.hpp"

#include <set>

namespace quxlang::detail
{
    struct output_binary_information_helpers
    {
        static auto parse_type_symbol_text(std::string const& text) -> type_symbol
        {
            parsers::parsing_context ctx = parsers::make_unlocated_parsing_context(text);
            type_symbol result = parsers::parse_type_symbol(ctx);
            if (ctx.iter_pos != ctx.iter_end)
            {
                throw syntax_compilation_error("Input not fully parsed");
            }
            return result;
        }

        static auto default_entry_functanoid() -> type_symbol
        {
            return parse_type_symbol_text("::main#()");
        }
    };
} // namespace quxlang::detail

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
            .module_names = {"main"},
            .main_functanoid = detail::output_binary_information_helpers::default_entry_functanoid(),
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
        main_functanoid = detail::output_binary_information_helpers::parse_type_symbol_text(config.main_functanoid.value_or("::main#()"));
    }
    else if (config.main_functanoid.has_value())
    {
        throw semantic_compilation_error("Output '" + input + "' of type unit_test_suite cannot configure main_functanoid");
    }

    std::vector< std::string > module_names;
    if (!config.modules.has_value())
    {
        module_names.push_back("main");
    }
    else if (config.modules->all_modules)
    {
        if (!config.modules->module_names.empty())
        {
            throw semantic_compilation_error("Output '" + input + "' cannot combine ALL with explicit module names");
        }
        if (config.type != output_kind::unit_test_suite)
        {
            throw semantic_compilation_error("Output '" + input + "' can select ALL modules only when its type is unit_test_suite");
        }
        module_names.reserve(target_config.module_configurations.size());
        for (std::pair< std::string const, module_configuration > const& module_entry : target_config.module_configurations)
        {
            module_names.push_back(module_entry.first);
        }
    }
    else
    {
        module_names = config.modules->module_names;
        std::set< std::string > const unique_module_names(module_names.begin(), module_names.end());
        if (unique_module_names.size() != module_names.size())
        {
            throw semantic_compilation_error("Output '" + input + "' cannot list a module more than once");
        }
    }

    if (module_names.empty())
    {
        throw semantic_compilation_error("Output '" + input + "' must select at least one module");
    }
    if (config.type != output_kind::unit_test_suite && module_names.size() != 1)
    {
        throw semantic_compilation_error("Output '" + input + "' must select exactly one module unless its type is unit_test_suite");
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
