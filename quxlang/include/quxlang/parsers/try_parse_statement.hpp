// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_TRY_PARSE_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_STATEMENT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_if_statement.hpp>
#include <quxlang/parsers/parse_for_statement.hpp>
#include <quxlang/parsers/parse_label_reference.hpp>
#include <quxlang/parsers/parse_return_statement.hpp>
#include <quxlang/parsers/parse_var_statement.hpp>
#include <quxlang/parsers/parse_while_statement.hpp>
#include <quxlang/parsers/statements.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_expression_statement.hpp>
#include <quxlang/parsers/fwd.hpp> // added forward declarations
#include <quxlang/parsers/parse_runtime_statement.hpp>
#include <quxlang/parsers/string_literal.hpp>

#include <utility>

namespace quxlang::parsers
{
    /// Parses a STATIC_EVAL statement and stores its expression for generation-time evaluation.
    /// STATIC_EVAL is used to mutate constexpr/compile time variables for generation loops,
    /// e.g. using STATIC_WHILE.
    inline function_static_eval_statement parse_static_eval_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "STATIC_EVAL"))
        {
            throw syntax_compilation_error("Expected 'STATIC_EVAL'");
        }

        skip_whitespace_and_comments(pos, end);
        function_static_eval_statement st;
        st.expr = parse_expression(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after STATIC_EVAL expression");
        }
        st.location = ctx.get_location_optional(begin, pos);
        return st;
    }

    inline std::optional< function_statement > try_parse_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        std::optional< function_statement > output;
        skip_whitespace_and_comments(pos, end);

        if (auto res = try_parse_function_block(ctx); res)
        {
            return std::move(*res);
        }

        auto kw = next_keyword(pos, end);

        if (skip_keyword_if_is(pos, end, "UNIMPLEMENTED"))
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after UNIMPLEMENTED statement");
            }
            function_unimplemented_statement st;
            st.location = ctx.get_location_optional(begin, pos);
            return std::optional< function_statement >{std::move(st)};
        }
        else if (skip_keyword_if_is(pos, end, "COMPILATION_ERROR"))
        {
            skip_whitespace_and_comments(pos, end);
            function_compilation_error_statement st;
            st.on_lower = skip_keyword_if_is(pos, end, "ON_LOWER");
            skip_whitespace_and_comments(pos, end);
            st.message = try_parse_string_literal(pos, end);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after COMPILATION_ERROR statement");
            }
            st.location = ctx.get_location_optional(begin, pos);
            return std::optional< function_statement >{std::move(st)};
        }
        else if (kw == "PLACE")
        {
            return parse_place_statement(ctx);
        }
        else if (kw == "DESTROY")
        {
            return parse_destroy_statement(ctx);
        }
        else if (kw == "RUNTIME")
        {
            return parse_runtime_statement(ctx);
        }
        else if (kw == "IF")
        {
            return parse_if_statement(ctx);
        }
        else if (kw == "STATIC_IF")
        {
            return parse_static_if_statement(ctx);
        }
        else if (kw == "VAR" || kw == "STATIC" || kw == "STATIC_VAR")
        {
            return parse_var_statement(ctx);
        }
        else if (kw == "STATIC_EVAL")
        {
            return parse_static_eval_statement(ctx);
        }
        else if (kw == "RETURN")
        {
            return parse_return_statement(ctx);
        }
        else if (skip_keyword_if_is(pos, end, "BREAK"))
        {
            auto label_name = try_parse_label_reference(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after BREAK statement");
            }
            function_break_statement st;
            st.label_name = std::move(label_name);
            st.location = ctx.get_location_optional(begin, pos);
            return std::optional< function_statement >{std::move(st)};
        }
        else if (skip_keyword_if_is(pos, end, "CONTINUE"))
        {
            auto label_name = try_parse_label_reference(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after CONTINUE statement");
            }
            function_continue_statement st;
            st.label_name = std::move(label_name);
            st.location = ctx.get_location_optional(begin, pos);
            return std::optional< function_statement >{std::move(st)};
        }
        else if (skip_keyword_if_is(pos, end, "GOTO"))
        {
            auto target = parse_label_reference(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after GOTO statement");
            }
            function_goto_statement st;
            st.target = std::move(target);
            st.location = ctx.get_location_optional(begin, pos);
            return std::optional< function_statement >{std::move(st)};
        }
        else if (skip_keyword_if_is(pos, end, "LABEL"))
        {
            auto name = parse_label_reference(ctx);
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ";"))
            {
                function_label_statement st;
                st.name = std::move(name);
                st.location = ctx.get_location_optional(begin, pos);
                return std::optional< function_statement >{std::move(st)};
            }
            function_label_block_statement st;
            st.name = std::move(name);
            st.block = parse_function_block(ctx);
            st.location = ctx.get_location_optional(begin, pos);
            return std::optional< function_statement >{std::move(st)};
        }
        else if (kw == "WHILE")
        {
            return parse_while_statement(ctx);
        }
        else if (kw == "FOR")
        {
            return parse_for_statement(ctx);
        }
        else if (kw == "STATIC_WHILE")
        {
            return parse_static_while_statement(ctx);
        }
        if (kw == "ASSERT")
        {
            return parse_assert_statement(ctx);
        }
        else if (auto expr_st = try_parse_expression_statement(ctx); expr_st)
        {
            return std::move(*expr_st);
        }

        return std::nullopt;
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_STATEMENT_HPP
