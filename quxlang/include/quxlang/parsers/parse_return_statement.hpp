//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef QUXLANG_PARSERS_PARSE_RETURN_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_RETURN_STATEMENT_HEADER_GUARD
#include <quxlang/data/function_return_statement.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_keyword.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    template < typename It >
    function_return_statement parse_return_statement(It& pos, It end)
    {
        if (!skip_keyword_if_is(pos, end, "RETURN"))
        {
            throw std::logic_error("Expected 'RETURN'");
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
            throw std::logic_error("Expected ';'");
        }

        return output;
    }

} // namespace quxlang::parsers

#endif // PARSE_RETURN_STATEMENT_HPP
