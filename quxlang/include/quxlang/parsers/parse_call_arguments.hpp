// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_CALL_ARGUMENTS_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_CALL_ARGUMENTS_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <quxlang/data/basic_types.hpp>
#include <quxlang/parsers/context.hpp>
#include <quxlang/parsers/function.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace quxlang::parsers
{
    namespace detail
    {
        expression parse_expression_impl(parsing_context& ctx);
    }

    /** Parses a comma-separated sequence of positional expressions through the requested closing symbol. */
    inline auto parse_positional_argument_sequence(parsing_context& ctx, std::string_view closing_symbol) -> std::vector< expression_arg >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        std::vector< expression_arg > result;

        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, closing_symbol))
        {
            return result;
        }

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "@"))
            {
                throw syntax_compilation_error("Named arguments are not allowed in a positional argument sequence");
            }

            parse_iterator begin = pos;
            expression_arg argument;
            argument.value = detail::parse_expression_impl(ctx);
            argument.location = ctx.get_location_optional(parse_iterator(begin), parse_iterator(pos));
            result.push_back(std::move(argument));

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, closing_symbol))
            {
                return result;
            }
            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' or '" + std::string(closing_symbol) + "' after positional argument");
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, closing_symbol))
            {
                throw syntax_compilation_error("Expected positional argument after ','");
            }
        }
    }

    /** Parses explicit named and positional arguments, or a single bare argument using the requested API name. */
    inline auto parse_call_argument_list(parsing_context& ctx, std::string_view closing_symbol, std::string_view bare_argument_name = "ARG") -> std::vector< expression_arg >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        std::vector< expression_arg > result;

        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, closing_symbol))
        {
            return result;
        }

        parse_iterator argument_begin = pos;
        bool const has_explicit_prefix = skip_symbol_if_is(pos, end, "@") || skip_symbol_if_is(pos, end, "%");
        pos = argument_begin;
        if (!has_explicit_prefix)
        {
            parse_iterator begin = pos;
            expression_arg argument;
            argument.name = std::string(bare_argument_name);
            argument.value = detail::parse_expression_impl(ctx);
            argument.location = ctx.get_location_optional(parse_iterator(begin), parse_iterator(pos));
            result.push_back(std::move(argument));

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, closing_symbol))
            {
                throw syntax_compilation_error("A bare argument must be the only argument; use '@name' or '% [...]' for explicit arguments");
            }
            return result;
        }

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "@"))
            {
                parse_iterator begin = pos;
                expression_arg argument;
                argument.name = parse_argument_name(pos, end);
                skip_whitespace_and_comments(pos, end);
                argument.value = detail::parse_expression_impl(ctx);
                argument.location = ctx.get_location_optional(parse_iterator(begin), parse_iterator(pos));
                result.push_back(std::move(argument));
            }
            else if (skip_symbol_if_is(pos, end, "%"))
            {
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, "["))
                {
                    throw syntax_compilation_error("Expected '[' after '%' in positional argument group");
                }
                std::vector< expression_arg > positional = parse_positional_argument_sequence(ctx, "]");
                result.insert(result.end(), std::make_move_iterator(positional.begin()), std::make_move_iterator(positional.end()));
            }
            else
            {
                throw syntax_compilation_error("Arguments must use '@name' or '% [...]'");
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, closing_symbol))
            {
                return result;
            }
            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' or '" + std::string(closing_symbol) + "' after call argument");
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, closing_symbol))
            {
                throw syntax_compilation_error("Expected explicit call argument after ','");
            }
        }
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_CALL_ARGUMENTS_HEADER_GUARD
