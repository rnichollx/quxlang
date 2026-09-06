// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_PARSE_DEFER_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_DEFER_STATEMENT_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/fwd.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_label_reference.hpp>

namespace quxlang::parsers
{
    /** Parses a deferred expression, labelled block, or eagerly evaluated callable. */
    inline auto parse_defer_statement(parsing_context& ctx) -> function_defer_statement
    {
        parse_iterator begin = ctx.iter_pos;
        if (!skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "DEFER"))
        {
            throw syntax_compilation_error("Expected DEFER");
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        function_defer_statement statement;
        if (skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "CALL"))
        {
            defer_call_action action;
            action.expr = parse_expression(ctx);
            action.location = ctx.get_location_optional(begin, ctx.iter_pos);
            statement.action = std::move(action);
        }
        else
        {
            std::optional< std::string > label = try_parse_label_reference(ctx);
            skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
            if (ctx.iter_pos != ctx.iter_end && *ctx.iter_pos == '{')
            {
                defer_block_action action;
                action.label_name = std::move(label);
                action.body = parse_function_block(ctx);
                action.location = ctx.get_location_optional(begin, ctx.iter_pos);
                statement.action = std::move(action);
                statement.location = ctx.get_location_optional(begin, ctx.iter_pos);
                return statement;
            }
            if (label.has_value())
            {
                throw syntax_compilation_error("A labelled DEFER requires a braced body");
            }
            defer_expression_action action;
            action.expr = parse_expression(ctx);
            action.location = ctx.get_location_optional(begin, ctx.iter_pos);
            statement.action = std::move(action);
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after DEFER expression");
        }
        statement.location = ctx.get_location_optional(begin, ctx.iter_pos);
        return statement;
    }
}

#endif
