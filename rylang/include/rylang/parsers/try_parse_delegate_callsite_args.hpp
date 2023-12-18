//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_DELEGATE_CALLSITE_ARGS_HPP
#define TRY_PARSE_DELEGATE_CALLSITE_ARGS_HPP
#include <rylang/data/expression.hpp>
#include <rylang/parsers/parse_whitespace_and_comments.hpp>
#include <rylang/parsers/skip_symbol_if_is.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::optional< std::vector< expression > > try_parse_delegate_callsite_args(It& pos, It end)
    {

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ":("))
        {
            return std::nullopt;
        }

        std::vector< expression > result;

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ")"))
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
            throw std::runtime_error("expected ',' or ')'");
        }

        return std::move(result);
    }
} // namespace rylang::parsers

#endif // TRY_PARSE_DELEGATE_CALLSITE_ARGS_HPP
