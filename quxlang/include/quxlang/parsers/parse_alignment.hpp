// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_ALIGNMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_ALIGNMENT_HEADER_GUARD

#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    /** Parses an optional ALIGN or ALIGNBITS declaration clause. */
    inline auto try_parse_alignment(parsing_context& ctx) -> std::optional< alignment_declaration >
    {
        std::optional< std::string_view > keyword = skip_keyword_if_one_of(ctx.iter_pos, ctx.iter_end, {"ALIGNBITS", "ALIGN"});
        if (!keyword.has_value())
        {
            return std::nullopt;
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, "("))
        {
            throw syntax_compilation_error("Expected '(' after alignment keyword");
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        expression value = parse_expression(ctx);
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after alignment expression");
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        return alignment_declaration{.value = std::move(value), .is_exponent = *keyword == "ALIGNBITS"};
    }
}

#endif // QUXLANG_PARSERS_PARSE_ALIGNMENT_HEADER_GUARD
