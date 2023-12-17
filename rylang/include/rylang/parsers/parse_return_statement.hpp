//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_RETURN_STATEMENT_HPP
#define PARSE_RETURN_STATEMENT_HPP
#include <rylang/data/function_return_statement.hpp>
#include <rylang/parsers/parse_whitespace_and_comments.hpp>
#include <rylang/parsers/parse_expression.hpp>
#include <rylang/parsers/parse_keyword.hpp>
#include <rylang/parsers/skip_symbol_if_is.hpp>

namespace rylang::parsers
{
    template < typename It >
    function_return_statement parse_return_statement(It& pos, It end)
    {
        if (!skip_keyword_if_is(pos, end, "RETURN"))
        {
            throw std::runtime_error("Expected 'RETURN'");
        }

        function_return_statement output;

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ";"))
        {
            return output;
        }

        output.expr = parse_expression(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::runtime_error("Expected ';'");
        }

        return output;
    }

} // namespace rylang::parsers

#endif // PARSE_RETURN_STATEMENT_HPP
