// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_RETURN_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_RETURN_STATEMENT_HEADER_GUARD
#include <quxlang/data/function_return_statement.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_keyword.hpp>
#include <quxlang/parsers/symbol.hpp>

namespace quxlang::parsers
{
    inline function_return_statement parse_return_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "RETURN"))
        {
            throw std::logic_error("Expected 'RETURN'");
        }

        function_return_statement output;

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ";"))
        {
            output.location = ctx.get_location_optional(begin, pos);
            return output;
        }

        output.expr = parse_expression(ctx);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ';'");
        }

        output.location = ctx.get_location_optional(begin, pos);
        return output;
    }

} // namespace quxlang::parsers

#endif // PARSE_RETURN_STATEMENT_HPP
