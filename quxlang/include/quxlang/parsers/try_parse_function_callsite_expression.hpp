// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/data/basic_types.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

#include <iterator>
#include <string_view>

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

    /** Parses explicit named and positional call arguments, or the single bare `@ARG` call form. */
    inline auto parse_call_argument_list(parsing_context& ctx, std::string_view closing_symbol) -> std::vector< expression_arg >
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
            argument.name = "ARG";
            argument.value = detail::parse_expression_impl(ctx);
            argument.location = ctx.get_location_optional(parse_iterator(begin), parse_iterator(pos));
            result.push_back(std::move(argument));

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, closing_symbol))
            {
                throw syntax_compilation_error("A bare call argument must be the only argument; use '@name' or '% [...]' for explicit arguments");
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
                throw syntax_compilation_error("Call arguments must use '@name' or '% [...]'");
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

    /** Returns whether a direct callee reference names an IEEE comparison keyword. */
    inline auto is_ieee_comparison_keyword_reference(expression const& callee) -> bool
    {
        if (!callee.template type_is< expression_symbol_reference >())
        {
            return false;
        }

        type_symbol const& symbol = callee.template get_as< expression_symbol_reference >().symbol;
        if (!symbol.template type_is< freebound_identifier >())
        {
            return false;
        }

        return is_builtin_ieee_comparison_name(symbol.template get_as< freebound_identifier >().name);
    }

    inline std::optional< expression_call > try_parse_function_callsite_expression(parsing_context& ctx, expression const& callee)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);

        parse_iterator begin = pos;
        if (!skip_symbol_if_is(pos, end, "("))
        {
            return std::nullopt;
        }

        expression_call result;

        if (is_ieee_comparison_keyword_reference(callee))
        {
            skip_whitespace_and_comments(pos, end);
            parse_iterator argument_begin = pos;
            bool const has_explicit_prefix = skip_symbol_if_is(pos, end, "@") || skip_symbol_if_is(pos, end, "%");
            pos = argument_begin;
            result.args = has_explicit_prefix ? parse_call_argument_list(ctx, ")") : parse_positional_argument_sequence(ctx, ")");
        }
        else
        {
            result.args = parse_call_argument_list(ctx, ")");
        }

        result.location = ctx.get_location_optional(parse_iterator(begin), parse_iterator(pos));
        return std::move(result);
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HPP
