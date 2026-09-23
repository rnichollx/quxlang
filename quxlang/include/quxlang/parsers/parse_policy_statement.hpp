// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_PARSE_POLICY_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_POLICY_STATEMENT_HEADER_GUARD

#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

namespace quxlang::parsers
{
    /** Parses a boolean policy block and an optional disabled-policy block. */
    inline auto parse_policy_statement(parsing_context& ctx) -> function_policy_statement
    {
        auto begin = ctx.iter_pos;
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "POLICY"))
        {
            throw syntax_compilation_error("Expected 'POLICY'");
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        function_policy_statement statement;
        if (skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "ASSERT_ENABLED"))
        {
            statement.policy = compilation_policy::policy_assert_enabled;
        }
        else if (skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "CHECK_BOUNDS"))
        {
            statement.policy = compilation_policy::policy_check_bounds;
        }
        else if (skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "CHECK_OVERFLOW"))
        {
            statement.policy = compilation_policy::policy_check_overflow;
        }
        else
        {
            throw syntax_compilation_error("Expected policy ASSERT_ENABLED, CHECK_BOUNDS, or CHECK_OVERFLOW");
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        statement.then_block = parse_function_block(ctx);
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "ELSE"))
        {
            skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
            statement.else_block = parse_function_block(ctx);
        }
        statement.location = ctx.get_location_optional(begin, ctx.iter_pos);
        return statement;
    }
}
#endif
