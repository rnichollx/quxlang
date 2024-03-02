//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_IF_STATEMENT_HPP
#define PARSE_IF_STATEMENT_HPP
#include <quxlang/data/function_if_statement.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/parse_expression.hpp>


namespace quxlang::parsers
{
    template < typename It >
    function_block parse_function_block(It& pos, It end);

    template < typename It >
    function_if_statement parse_if_statement(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "IF"))
        {
            throw std::runtime_error("Expected 'IF'");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::runtime_error("Expected '('");
        }

        function_if_statement if_statement;

        if_statement.condition = parse_expression(pos, end);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::runtime_error("Expected ')'");
        }

        skip_whitespace_and_comments(pos, end);

        if_statement.then_block = parse_function_block(pos, end);

        skip_wsc(pos, end);

        if (skip_keyword_if_is(pos, end, "ELSE"))
        {
            skip_wsc(pos, end);
            if_statement.else_block = parse_function_block(pos, end);
        }

        return if_statement;
    }

} // namespace quxlang

#endif // PARSE_IF_STATEMENT_HPP
