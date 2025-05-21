// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_IF_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_IF_STATEMENT_HEADER_GUARD
#include <quxlang/data/function_if_statement.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

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
            throw std::logic_error("Expected 'IF'");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '('");
        }

        function_if_statement if_statement;

        if_statement.condition = parse_expression(pos, end);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::logic_error("Expected ')'");
        }

        skip_whitespace_and_comments(pos, end);

        if_statement.then_block = parse_function_block(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (skip_keyword_if_is(pos, end, "ELSE"))
        {
            skip_whitespace_and_comments(pos, end);
            if_statement.else_block = parse_function_block(pos, end);
        }

        return if_statement;
    }

    template < typename It >
    function_assert_statement parse_assert_statement(It& pos, It end)
    {
        auto beg = pos;
        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "ASSERT"))
        {
            throw std::logic_error("Expected 'ASSERT'");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '('");
        }

        function_assert_statement asrt_statement;

        asrt_statement.condition = parse_expression(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end , ","))
        {
            skip_whitespace_and_comments(pos, end);
            asrt_statement.tagline = try_parse_string_literal(pos, end).value_or("NO_MESSAGE");

            std::string tag = asrt_statement.tagline.value();
            // TODO: Fix this up a bit
        }
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::logic_error("Expected ')'");
        }



        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ';'");
        }

        asrt_statement.location.set(pos, end);
        return asrt_statement;
    }
} // namespace quxlang::parsers

#endif // PARSE_IF_STATEMENT_HPP
