// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_RETURN_UNEQUAL_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_RETURN_UNEQUAL_STATEMENT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    /** Parses a RETURN_UNEQUAL statement containing its left and right expressions. */
    inline function_return_unequal_statement parse_return_unequal_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "RETURN_UNEQUAL"))
        {
            throw syntax_compilation_error("Expected 'RETURN_UNEQUAL'");
        }

        skip_whitespace_and_comments(pos, end);
        function_return_unequal_statement output;
        output.lhs = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ","))
        {
            throw syntax_compilation_error("Expected ',' between RETURN_UNEQUAL expressions");
        }

        skip_whitespace_and_comments(pos, end);
        output.rhs = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after RETURN_UNEQUAL statement");
        }

        output.location = ctx.get_location_optional(begin, pos);
        return output;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_RETURN_UNEQUAL_STATEMENT_HEADER_GUARD
