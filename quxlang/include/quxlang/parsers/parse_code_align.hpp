// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_PARSE_CODE_ALIGN_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_CODE_ALIGN_HEADER_GUARD

#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

namespace quxlang::parsers
{
    /** Parses HINT_CODE_ALIGN and its parenthesized constant expression. */
    inline auto parse_code_align(parsing_context& ctx) -> function_code_align_statement
    {
        auto begin = ctx.iter_pos;
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "HINT_CODE_ALIGN"))
        {
            throw syntax_compilation_error("Expected HINT_CODE_ALIGN");
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, "("))
        {
            throw syntax_compilation_error("Expected '(' after HINT_CODE_ALIGN");
        }
        function_code_align_statement result;
        result.alignment = parse_expression(ctx);
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after HINT_CODE_ALIGN expression");
        }
        result.location = ctx.get_location_optional(begin, ctx.iter_pos);
        return result;
    }
}
#endif
