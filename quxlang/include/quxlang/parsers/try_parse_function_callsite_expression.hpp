// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/data/basic_types.hpp>
#include <quxlang/parsers/parse_call_arguments.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
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
