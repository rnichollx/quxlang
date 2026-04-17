// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_EXPRESSION_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_EXPRESSION_STATEMENT_HEADER_GUARD


#include <quxlang/parsers/try_parse_expression.hpp>

#include <utility>

namespace quxlang::parsers
{
    inline std::optional< function_expression_statement > try_parse_expression_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        std::optional< expression > expr;
        if (auto expr = try_parse_expression(ctx); expr)
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ';' after expression");
            }
            function_expression_statement result;
            result.expr = std::move(*expr);
            result.location = ctx.get_location_optional(begin, pos);
            return std::move(result);
        }
        return std::nullopt;
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_EXPRESSION_STATEMENT_HPP
