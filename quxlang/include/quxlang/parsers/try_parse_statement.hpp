// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_TRY_PARSE_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_STATEMENT_HEADER_GUARD
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_if_statement.hpp>
#include <quxlang/parsers/parse_return_statement.hpp>
#include <quxlang/parsers/parse_var_statement.hpp>
#include <quxlang/parsers/parse_while_statement.hpp>
#include <quxlang/parsers/statements.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_expression_statement.hpp>
#include <quxlang/parsers/fwd.hpp> // added forward declarations
#include <quxlang/parsers/parse_runtime_statement.hpp>

namespace quxlang::parsers
{
    inline std::optional< function_statement > try_parse_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        std::optional< function_statement > output;
        skip_whitespace_and_comments(pos, end);

        if (auto res = try_parse_function_block(ctx); res)
        {
            return *res;
        }

        if (skip_keyword_if_is(pos, end, "UNIMPLEMENTED"))
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ';' after UNIMPLEMENTED statement");
            }
            function_unimplemented_statement st;
            st.location = ctx.get_location_optional(begin, pos);
            return st;
        }
        else if (next_keyword(pos, end) == "PLACE")
        {
            return parse_place_statement(ctx);
        }
        else if (next_keyword(pos, end) == "DESTROY")
        {
            return parse_destroy_statement(ctx);
        }
        else if (next_keyword(pos, end) == "RUNTIME")
        {
            return parse_runtime_statement(ctx);
        }
        else if (next_keyword(pos, end) == "IF")
        {
            return parse_if_statement(ctx);
        }
        else if (next_keyword(pos, end) == "VAR")
        {
            return parse_var_statement(ctx);
        }
        else if (next_keyword(pos, end) == "RETURN")
        {
            return parse_return_statement(ctx);
        }
        else if (next_keyword(pos, end) == "WHILE")
        {
            return parse_while_statement(ctx);
        }
        if (next_keyword(pos, end) == "ASSERT")
        {
            return parse_assert_statement(ctx);
        }
        else if (auto expr_st = try_parse_expression_statement(ctx); expr_st)
        {
            return *expr_st;
        }

        return std::nullopt;
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_STATEMENT_HPP
