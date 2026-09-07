// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_PARSE_EXCEPTION_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_EXCEPTION_STATEMENT_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/fwd.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>

namespace quxlang::parsers
{
    /** Parses a throw expression followed by its required statement terminator. */
    inline auto parse_throw_statement(parsing_context& ctx) -> function_throw_statement
    {
        parse_iterator begin = ctx.iter_pos;
        if (!skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "THROW"))
        {
            throw syntax_compilation_error("Expected THROW");
        }
        function_throw_statement statement;
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "UNWIND_OUT_OF_MEMORY"))
        {
            statement.expr = parse_expression(ctx);
        }
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        if (!skip_symbol_if_is(ctx.iter_pos, ctx.iter_end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after THROW expression");
        }
        statement.location = ctx.get_location_optional(begin, ctx.iter_pos);
        return statement;
    }

    /** Parses a protected block and its ordered typed, sentinel, or default catches. */
    inline auto parse_try_statement(parsing_context& ctx) -> function_try_statement
    {
        parse_iterator begin = ctx.iter_pos;
        if (!skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "TRY"))
        {
            throw syntax_compilation_error("Expected TRY");
        }
        function_try_statement statement;
        statement.body = parse_function_block(ctx);
        bool has_default = false;
        while (true)
        {
            skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
            parse_iterator handler_begin = ctx.iter_pos;
            if (!skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "CATCH"))
            {
                break;
            }
            if (has_default)
            {
                throw syntax_compilation_error("CATCH DEFAULT must be the last handler");
            }
            skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
            if (skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "DEFAULT"))
            {
                function_default_catch handler;
                handler.body = parse_function_block(ctx);
                handler.location = ctx.get_location_optional(handler_begin, ctx.iter_pos);
                statement.handlers.emplace_back(std::move(handler));
                has_default = true;
            }
            else if (skip_keyword_if_is(ctx.iter_pos, ctx.iter_end, "UNWIND_OUT_OF_MEMORY"))
            {
                function_unwind_out_of_memory_catch handler;
                handler.body = parse_function_block(ctx);
                handler.location = ctx.get_location_optional(handler_begin, ctx.iter_pos);
                statement.handlers.emplace_back(std::move(handler));
            }
            else
            {
                function_typed_catch handler;
                handler.binding_name = parse_identifier(ctx.iter_pos, ctx.iter_end);
                if (handler.binding_name.empty())
                {
                    throw syntax_compilation_error("Expected a binding name, DEFAULT, or UNWIND_OUT_OF_MEMORY after CATCH");
                }
                handler.reference_type = parse_type_symbol(ctx);
                handler.body = parse_function_block(ctx);
                handler.location = ctx.get_location_optional(handler_begin, ctx.iter_pos);
                statement.handlers.emplace_back(std::move(handler));
            }
        }
        if (statement.handlers.empty())
        {
            throw syntax_compilation_error("TRY requires at least one CATCH handler");
        }
        statement.location = ctx.get_location_optional(begin, ctx.iter_pos);
        return statement;
    }
}

#endif
