// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/data/basic_types.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    namespace detail
    {
        expression parse_expression_impl(parsing_context& ctx);
    }

    inline expression_arg parse_expression_arg(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        expression_arg result;
        if (skip_symbol_if_is(pos, end, "@"))
        {
            result.name = parse_argument_name(pos, end);
            skip_whitespace_and_comments(pos, end);
        }

        result.value = detail::parse_expression_impl(ctx);
        result.location = ctx.get_location_optional(parse_iterator(begin), parse_iterator(pos));
        return result;
    }

    inline std::optional< expression_call > try_parse_function_callsite_expression(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);

        auto begin = pos;
        if (!skip_symbol_if_is(pos, end, "("))
        {
            return std::nullopt;
        }

        expression_call result;

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ")"))
        {
            return std::move(result);
        }
    get_arg:

        skip_whitespace_and_comments(pos, end);
        expression_arg expr = parse_expression_arg(ctx);
        result.args.push_back(std::move(expr));

        if (skip_symbol_if_is(pos, end, ","))
        {
            goto get_arg;
        }

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("expected ',' or ')'");
        }

        result.location = ctx.get_location_optional(parse_iterator(begin), parse_iterator(pos));
        return std::move(result);
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HPP
