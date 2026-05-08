/**
 * @file parse_function_block.hpp
 * @brief Contains parsers for function block constructs in Quxlang.
 *
 * This file defines functions to parse function blocks enclosed in braces '{' and '}'.
 * It utilizes helper functions to skip whitespace and comments and converts the 
 * input into a structured function_block.
 */

// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <utility>
#include <quxlang/data/function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_statement.hpp>
#include <quxlang/parsers/fwd.hpp> // added forward declarations

namespace quxlang::parsers
{
    inline std::optional<function_block> try_parse_function_block(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        function_block body;
        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "{"))
        {
           return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, "}"))
        {
            body.location = ctx.get_location_optional(begin, pos);
            return std::move(body);
        }

        std::optional< function_statement > statement;

        while ((statement = try_parse_statement(ctx)))
        {
            body.statements.push_back(std::move(*statement));
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "}"))
            {
                body.location = ctx.get_location_optional(begin, pos);
                return std::move(body);
            }
        }

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, "}"))
        {
            body.location = ctx.get_location_optional(begin, pos);
            return std::move(body);
        }
        throw syntax_compilation_error("Expected '}' or statement");
    }

    inline function_block parse_function_block(parsing_context& ctx)
    {
        auto fb = try_parse_function_block(ctx);
        if (fb)
        {
            return std::move(*fb);
        }
        throw syntax_compilation_error("Expected a function block");
    }

} // namespace quxlang::parsers

#endif // PARSE_FUNCTION_BODY_HPP
