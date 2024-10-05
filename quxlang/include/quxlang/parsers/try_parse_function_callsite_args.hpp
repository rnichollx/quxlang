//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_ARGS_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_CALLSITE_ARGS_HEADER_GUARD
#include <quxlang/data/expression.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< std::vector< expression > > try_parse_function_callsite_args(It& pos, It end)
    {

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            return std::nullopt;
        }

        std::vector< expression > result;

        skip_whitespace_and_comments(pos, end);

        if (parsers::skip_symbol_if_is(pos, end, ")"))
        {
            return std::move(result);
        }
    get_arg:

        expression expr = parsers::parse_expression(pos, end);
        result.push_back(std::move(expr));

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
} // namespace quxlang

#endif // TRY_PARSE_FUNCTION_CALLSITE_ARGS_HPP
