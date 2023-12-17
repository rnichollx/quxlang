//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HPP
#define TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HPP
#include <rylang/data/expression_call.hpp>
#include <rylang/parsers/parse_expression.hpp>
#include <rylang/parsers/parse_whitespace_and_comments.hpp>
#include <rylang/parsers/skip_symbol_if_is.hpp>

namespace rylang::parsers
{
    template < typename It >
    expression parse_expression(It& pos, It end);

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

        expression expr = parse_expression(pos, end);
        result.args.push_back(std::move(expr));

        if (skip_symbol_if_is(pos, end, ","))
        {
            goto get_arg;
        }

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::runtime_error("expected ',' or ')'");
        }

        return std::move(result);
    }
} // namespace rylang

#endif // TRY_PARSE_FUNCTION_CALLSITE_EXPRESSION_HPP
