// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HEADER_GUARD
#include <quxlang/data/expression_call.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    template < typename It >
    expression parse_expression(It& pos, It end);

    template < typename It >
    expression_arg parse_expression_arg(It& pos, It end)
    {
        expression_arg result;
        if (skip_symbol_if_is(pos, end, "@"))
        {
            result.name = parse_argument_name(pos, end);
            skip_whitespace_and_comments(pos, end);
        }

        result.value = parse_expression(pos, end);
        return result;
    }

    template < typename It >
    std::optional< expression_call > try_parse_function_callsite_expression(It& pos, It end)
    {

        skip_whitespace_and_comments(pos, end);

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
        expression_arg expr = parse_expression_arg(pos, end);
        result.args.push_back(std::move(expr));

        if (skip_symbol_if_is(pos, end, ","))
        {
            goto get_arg;
        }

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::logic_error("expected ',' or ')'");
        }

        return std::move(result);
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HPP
