// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/parsers/parse_alignment.hpp>
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/peek_symbol.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>

namespace quxlang::parsers
{
    /** Parses a local variable declaration with an explicit or deduced type. */
    inline function_var_statement parse_var_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        function_var_statement var_statement;

        var_statement.is_pack = skip_keyword_if_is(pos, end, "PACK");
        bool is_auto = skip_keyword_if_is(pos, end, "AUTO");
        bool is_var = var_statement.is_pack || is_auto || skip_keyword_if_is(pos, end, "VAR");
        if (!is_var && skip_keyword_if_is(pos, end, "STATIC_VAR"))
        {
            var_statement.static_kind = function_static_kind::mutable_;
        }
        else if (!is_var && skip_keyword_if_is(pos, end, "STATIC"))
        {
            var_statement.static_kind = function_static_kind::constant;
        }
        else if (!is_var)
        {
            throw syntax_compilation_error("Expected 'AUTO', 'VAR', 'STATIC', or 'STATIC_VAR'");
        }

        skip_whitespace_and_comments(pos, end);

        var_statement.name = parse_identifier(pos, end);

        skip_whitespace_and_comments(pos, end);

        bool emit_field = skip_keyword_if_is(pos, end, "EMIT");
        if (var_statement.is_pack || emit_field)
        {
            var_statement.type = auto_temploidic{};
            expression_pack_operation operation;
            if (emit_field || skip_keyword_if_is(pos, end, "FISSION"))
            {
                operation.operation = emit_field ? pack_operation::emit : pack_operation::fission;
                skip_whitespace_and_comments(pos, end);
                if (!skip_keyword_if_is(pos, end, emit_field ? "FIELD" : "FIELDS"))
                    throw syntax_compilation_error("Expected pack field selection");
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, "(")) throw syntax_compilation_error("Expected '(' before pack fields");
                std::vector< expression > selectors;
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    do
                    {
                        selectors.push_back(parse_expression(ctx));
                        skip_whitespace_and_comments(pos, end);
                    } while (skip_symbol_if_is(pos, end, ","));
                    if (!skip_symbol_if_is(pos, end, ")")) throw syntax_compilation_error("Expected ')' after pack fields");
                }
                skip_whitespace_and_comments(pos, end);
                if (!skip_keyword_if_is(pos, end, "FROM")) throw syntax_compilation_error("Expected FROM after pack fields");
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, "(")) throw syntax_compilation_error("Expected '(' after FROM");
                operation.operands.push_back(parse_expression(ctx));
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")")) throw syntax_compilation_error("Expected ')' after pack source");
                for (expression& selector : selectors) operation.operands.push_back(std::move(selector));
                var_statement.equals_initializer = std::move(operation);
            }
            else if (skip_keyword_if_is(pos, end, "FUSE"))
            {
                operation.operation = pack_operation::fuse;
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, "(")) throw syntax_compilation_error("Expected '(' after FUSE");
                do
                {
                    operation.operands.push_back(parse_expression(ctx));
                    skip_whitespace_and_comments(pos, end);
                } while (skip_symbol_if_is(pos, end, ","));
                if (!skip_symbol_if_is(pos, end, ")")) throw syntax_compilation_error("Expected ')' after FUSE operands");
                var_statement.equals_initializer = std::move(operation);
            }
            else
            {
                var_statement.equals_initializer = parse_expression(ctx);
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";")) throw syntax_compilation_error("Expected semicolon after pack operation");
            var_statement.location = ctx.get_location_optional(begin, pos);
            return var_statement;
        }

        if (is_auto || (is_var && peek_symbol(pos, end).starts_with(":=")))
        {
            var_statement.type = auto_temploidic{};
        }
        else
        {
            var_statement.type = parse_type_symbol(ctx);
        }

        skip_whitespace_and_comments(pos, end);
        var_statement.alignment = try_parse_alignment(ctx);

        if (!is_auto && skip_symbol_if_is(pos, end, ":("))
        {
            var_statement.initializers = parse_call_argument_list(ctx, ")");
        }
        else if (!is_auto && skip_symbol_if_is(pos, end, ":["))
        {
            var_statement.initializers = parse_positional_argument_sequence(ctx, "]");
        }
        else if (skip_symbol_if_is(pos, end, ":="))
        {
            skip_whitespace_and_comments(pos, end);
            var_statement.equals_initializer = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';'");
        }

        if (is_auto && !var_statement.equals_initializer.has_value())
        {
            throw syntax_compilation_error("AUTO variable declarations require a := initializer");
        }

        var_statement.location = ctx.get_location_optional(begin, pos);
        return var_statement;
    }

} // namespace quxlang

#endif // PARSE_VAR_STATEMENT_HPP
