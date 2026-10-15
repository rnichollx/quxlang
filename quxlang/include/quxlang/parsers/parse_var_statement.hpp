// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/parsers/parse_alignment.hpp>
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>

namespace quxlang::parsers
{
    /** Parses a local variable declaration, including the AUTO shorthand. */
    inline function_var_statement parse_var_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        function_var_statement var_statement;

        bool is_auto = skip_keyword_if_is(pos, end, "AUTO");
        bool is_var = is_auto || skip_keyword_if_is(pos, end, "VAR");
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

        if (is_auto)
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
