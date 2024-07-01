//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_WHILE_STATEMENT_HPP
#define PARSE_WHILE_STATEMENT_HPP
#include <quxlang/data/function_while_statement.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_expression.hpp>

namespace quxlang::parsers
{
    template < typename It >
    function_while_statement parse_while_statement(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "WHILE"))
        {
            throw std::logic_error("Expected 'WHILE'");
        }
        function_while_statement output;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '('");
        }
        output.condition = parse_expression(pos, end);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::logic_error("Expected ')'");
        }
        skip_whitespace_and_comments(pos, end);
        output.loop_block = parse_function_block(pos, end);
        return output;
    }
} // namespace quxlang::parsers

#endif // PARSE_WHILE_STATEMENT_HPP
