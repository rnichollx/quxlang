// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_WHILE_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_WHILE_STATEMENT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/data/function_while_statement.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_label_reference.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_expression.hpp>

namespace quxlang::parsers
{
    function_block parse_function_block(parsing_context& ctx);

    inline function_while_statement parse_while_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "WHILE"))
        {
            throw syntax_compilation_error("Expected 'WHILE'");
        }
        function_while_statement output;
        output.label_name = try_parse_label_reference(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '('");
        }
        output.condition = parse_expression(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')'");
        }
        skip_whitespace_and_comments(pos, end);
        output.loop_block = parse_function_block(ctx);
        output.location = ctx.get_location_optional(begin, pos);
        return output;
    }

    /// Parses a STATIC_WHILE statement whose condition and body execute during generation.
    inline function_static_while_statement parse_static_while_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "STATIC_WHILE"))
        {
            throw syntax_compilation_error("Expected 'STATIC_WHILE'");
        }
        function_static_while_statement output;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after STATIC_WHILE");
        }
        output.condition = parse_expression(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after STATIC_WHILE condition");
        }
        skip_whitespace_and_comments(pos, end);
        output.loop_block = parse_function_block(ctx);
        output.location = ctx.get_location_optional(begin, pos);
        return output;
    }

} // namespace quxlang::parsers

#endif // PARSE_WHILE_STATEMENT_HPP
