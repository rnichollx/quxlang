//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_EXPRESSION_STATEMENT_HPP
#define TRY_PARSE_EXPRESSION_STATEMENT_HPP


#include <quxlang/parsers/try_parse_expression.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< function_expression_statement > try_parse_expression_statement(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        std::string remaining = std::string(pos, end);
        std::optional< expression > expr;
        if (auto expr = try_parse_expression(pos, end); expr)
        {
            skip_whitespace_and_comments(pos, end);
            remaining = std::string(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ';' after expression");
            }
            function_expression_statement result;
            result.expr = std::move(expr.value());
            return result;
        }
        return std::nullopt;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_EXPRESSION_STATEMENT_HPP
