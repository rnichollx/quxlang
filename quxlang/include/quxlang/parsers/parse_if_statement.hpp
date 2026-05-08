// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_IF_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_IF_STATEMENT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/data/function_if_statement.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <utility>

namespace quxlang::parsers
{
    function_block parse_function_block(parsing_context& ctx);

    template < bool Try >
    auto parse_if_statement_ext(parsing_context& ctx) -> std::conditional_t< Try, std::optional< function_if_statement >, function_if_statement >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "IF"))
        {
            if constexpr (Try)
            {
                return std::nullopt;
            }
            else
            {
                throw syntax_compilation_error("Expected 'IF'");
            }
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '('");
        }

        function_if_statement if_statement;

        if_statement.condition = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')'");
        }

        skip_whitespace_and_comments(pos, end);

        if_statement.then_block = parse_function_block(ctx);

        skip_whitespace_and_comments(pos, end);

        if (skip_keyword_if_is(pos, end, "ELSE"))
        {
            skip_whitespace_and_comments(pos, end);
            auto try_if = parse_if_statement_ext< true >(ctx);
            if (try_if.has_value())
            {
                function_block block;
                block.statements.push_back(std::move(*try_if));
                if_statement.else_block = std::move(block);
            }
            else
            {
                if_statement.else_block = parse_function_block(ctx);
            }
        }
        else if (next_keyword(pos, end) == "STATIC_ELSE")
        {
            throw syntax_compilation_error("Expected 'ELSE' after IF, not 'STATIC_ELSE'");
        }

        if_statement.location = ctx.get_location_optional(begin, pos);
        return if_statement;
    }

    inline auto parse_if_statement(parsing_context& ctx) -> function_if_statement
    {
        return parse_if_statement_ext< false >(ctx);
    }

    inline std::optional< function_if_statement > try_parse_if_statement(parsing_context& ctx)
    {
        return parse_if_statement_ext< true >(ctx);
    }

    /// Parses STATIC_IF and optionally returns std::nullopt when Try is true and no STATIC_IF is present.
    template < bool Try >
    auto parse_static_if_statement_ext(parsing_context& ctx) -> std::conditional_t< Try, std::optional< function_static_if_statement >, function_static_if_statement >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "STATIC_IF"))
        {
            if constexpr (Try)
            {
                return std::nullopt;
            }
            else
            {
                throw syntax_compilation_error("Expected 'STATIC_IF'");
            }
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after STATIC_IF");
        }

        function_static_if_statement if_statement;
        if_statement.condition = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after STATIC_IF condition");
        }

        skip_whitespace_and_comments(pos, end);
        if_statement.then_block = parse_function_block(ctx);

        skip_whitespace_and_comments(pos, end);
        if (skip_keyword_if_is(pos, end, "STATIC_ELSE"))
        {
            skip_whitespace_and_comments(pos, end);
            auto try_if = parse_static_if_statement_ext< true >(ctx);
            if (try_if.has_value())
            {
                function_block block;
                block.statements.push_back(std::move(*try_if));
                if_statement.else_block = std::move(block);
            }
            else
            {
                if_statement.else_block = parse_function_block(ctx);
            }
        }
        else if (next_keyword(pos, end) == "ELSE")
        {
            throw syntax_compilation_error("Expected 'STATIC_ELSE' after STATIC_IF, not 'ELSE'");
        }

        if_statement.location = ctx.get_location_optional(begin, pos);
        return if_statement;
    }

    /// Parses a required STATIC_IF statement, including optional STATIC_ELSE or STATIC_ELSE STATIC_IF.
    inline auto parse_static_if_statement(parsing_context& ctx) -> function_static_if_statement
    {
        return parse_static_if_statement_ext< false >(ctx);
    }

    /// Attempts to parse a STATIC_IF statement without consuming input when none is present.
    inline std::optional< function_static_if_statement > try_parse_static_if_statement(parsing_context& ctx)
    {
        return parse_static_if_statement_ext< true >(ctx);
    }

    inline function_assert_statement parse_assert_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "ASSERT"))
        {
            throw syntax_compilation_error("Expected 'ASSERT'");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '('");
        }

        function_assert_statement asrt_statement;

        asrt_statement.condition = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ","))
        {
            skip_whitespace_and_comments(pos, end);
            asrt_statement.tagline = try_parse_string_literal(pos, end).value_or("NO_MESSAGE");
        }
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')'");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';'");
        }

        asrt_statement.location = ctx.get_location_optional(begin, pos);
        return asrt_statement;
    }

} // namespace quxlang::parsers

#endif // PARSE_IF_STATEMENT_HPP
