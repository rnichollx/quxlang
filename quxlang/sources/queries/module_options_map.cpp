// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/module_options_map_spec.hpp>

#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>

#include <stdexcept>

namespace
{
    auto parse_type_symbol_text(std::string const& text) -> quxlang::type_symbol
    {
        auto ctx = quxlang::parsers::make_unlocated_parsing_context(text);
        auto result = quxlang::parsers::parse_type_symbol(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw std::logic_error("Input not fully parsed");
        }
        return result;
    }
}

rpnx::querygraph::coroutine< quxlang::module_options_map_spec > quxlang::module_options_map_impl(std::monostate)
{
    auto const option_strings = co_await rpnx::querygraph::request< module_option_strings_map_query >(std::monostate{});
    std::map< type_symbol, std::string > output;

    for (auto const& [module_name, option_values] : option_strings)
    {
        type_symbol const module_context = absolute_module_reference{.module_name = module_name};

        for (auto const& [option_name, option_value] : option_values)
        {
            auto parsed_option = parse_type_symbol_text(option_name);
            auto resolved_option = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                .context = module_context,
                .type = parsed_option,
            });
            if (!resolved_option.has_value())
            {
                throw std::logic_error("Configured option '" + option_name + "' in module '" + module_name + "' did not resolve to a symbol");
            }

            auto kind = co_await rpnx::querygraph::request< symbol_type_query >(*resolved_option);
            if (kind != symbol_kind::option)
            {
                throw std::logic_error("Configured option '" + option_name + "' in module '" + module_name + "' resolved to non-option symbol " + to_string(*resolved_option));
            }

            auto [it, inserted] = output.emplace(*resolved_option, option_value);
            if (!inserted)
            {
                throw std::logic_error("Duplicate configured option for symbol " + to_string(*resolved_option));
            }
        }
    }

    co_return output;
}
