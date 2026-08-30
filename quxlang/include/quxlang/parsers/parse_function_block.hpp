/**
 * @file parse_function_block.hpp
 * @brief Contains parsers for function block constructs in Quxlang.
 *
 * This file defines functions to parse function blocks enclosed in braces '{' and '}'.
 * It utilizes helper functions to skip whitespace and comments and converts the 
 * input into a structured function_block.
 */

// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <cstddef>
#include <utility>
#include <quxlang/data/function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_statement.hpp>
#include <quxlang/parsers/fwd.hpp> // added forward declarations

namespace quxlang::parsers
{
    namespace detail
    {
        /** Moves each continuation-form VISIT's following siblings into its specialized body. */
        inline auto normalize_visit_continuations(function_block& body) -> void
        {
            for (std::size_t statement_index = body.statements.size(); statement_index-- != 0;)
            {
                function_statement& statement = body.statements[statement_index];
                if (!statement.type_is< function_visit_statement >())
                {
                    continue;
                }

                function_visit_statement& visit_statement = statement.as< function_visit_statement >();
                bool const continuation = visit_statement.form == function_visit_form::variable_continuation ||
                                          visit_statement.form == function_visit_form::named_continuation ||
                                          visit_statement.form == function_visit_form::extended_named_continuation;
                if (!continuation)
                {
                    continue;
                }

                visit_statement.body.location = body.location;
                for (std::size_t sibling_index = statement_index + 1; sibling_index < body.statements.size(); ++sibling_index)
                {
                    visit_statement.body.statements.push_back(std::move(body.statements[sibling_index]));
                }
                body.statements.resize(statement_index + 1);
            }
        }
    } // namespace detail

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
            detail::normalize_visit_continuations(body);
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
                detail::normalize_visit_continuations(body);
                return std::move(body);
            }
        }

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, "}"))
        {
            body.location = ctx.get_location_optional(begin, pos);
            detail::normalize_visit_continuations(body);
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
