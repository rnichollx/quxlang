// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FOR_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FOR_STATEMENT_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_label_reference.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace quxlang::parsers
{
    function_block parse_function_block(parsing_context& ctx);

    inline auto parse_for_parenthesized_expression(parsing_context& ctx, std::string_view clause_name) -> expression
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after " + std::string(clause_name));
        }

        auto output = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after " + std::string(clause_name));
        }
        return output;
    }

    inline auto parse_for_parenthesized_identifier(parsing_context& ctx, std::string_view clause_name) -> std::string
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after " + std::string(clause_name));
        }

        skip_whitespace_and_comments(pos, end);
        auto output = parse_identifier(pos, end);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after " + std::string(clause_name));
        }
        return output;
    }

    template < typename T >
    inline auto assign_for_clause_once(std::optional< T >& target, T value, std::string_view clause_name) -> void
    {
        if (target.has_value())
        {
            throw syntax_compilation_error("Duplicate FOR " + std::string(clause_name) + " clause");
        }
        target = std::move(value);
    }

    inline auto parse_for_statement(parsing_context& ctx) -> function_for_statement
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "FOR"))
        {
            throw syntax_compilation_error("Expected 'FOR'");
        }

        function_for_statement output;
        output.label_name = try_parse_label_reference(ctx);

        while (true)
        {
            skip_whitespace_and_comments(pos, end);

            if (skip_keyword_if_is(pos, end, "LOOP"))
            {
                output.loop_block = parse_function_block(ctx);
                skip_whitespace_and_comments(pos, end);
                skip_symbol_if_is(pos, end, ";");
                output.location = ctx.get_location_optional(begin, pos);
                return output;
            }
            if (skip_keyword_if_is(pos, end, "INIT"))
            {
                assign_for_clause_once(output.init_block, parse_function_block(ctx), "INIT");
            }
            else if (skip_keyword_if_is(pos, end, "EVAL"))
            {
                assign_for_clause_once(output.eval_block, parse_function_block(ctx), "EVAL");
            }
            else if (skip_keyword_if_is(pos, end, "TEST"))
            {
                assign_for_clause_once(output.test_condition, parse_for_parenthesized_expression(ctx, "TEST"), "TEST");
            }
            else if (skip_keyword_if_is(pos, end, "POSTTEST"))
            {
                assign_for_clause_once(output.posttest_condition, parse_for_parenthesized_expression(ctx, "POSTTEST"), "POSTTEST");
            }
            else if (skip_keyword_if_is(pos, end, "STEP"))
            {
                assign_for_clause_once(output.step_block, parse_function_block(ctx), "STEP");
            }
            else if (skip_keyword_if_is(pos, end, "ITER"))
            {
                assign_for_clause_once(output.iter_name, parse_for_parenthesized_identifier(ctx, "ITER"), "ITER");
            }
            else if (skip_keyword_if_is(pos, end, "VALUE"))
            {
                assign_for_clause_once(output.value_name, parse_for_parenthesized_identifier(ctx, "VALUE"), "VALUE");
            }
            else if (skip_keyword_if_is(pos, end, "INDEX"))
            {
                assign_for_clause_once(output.index_name, parse_for_parenthesized_identifier(ctx, "INDEX"), "INDEX");
            }
            else if (skip_keyword_if_is(pos, end, "ITEM"))
            {
                assign_for_clause_once(output.item_name, parse_for_parenthesized_identifier(ctx, "ITEM"), "ITEM");
            }
            else if (skip_keyword_if_is(pos, end, "IN"))
            {
                assign_for_clause_once(output.in_expr, parse_for_parenthesized_expression(ctx, "IN"), "IN");
            }
            else if (skip_keyword_if_is(pos, end, "START"))
            {
                assign_for_clause_once(output.start_expr, parse_for_parenthesized_expression(ctx, "START"), "START");
            }
            else if (skip_keyword_if_is(pos, end, "END"))
            {
                assign_for_clause_once(output.end_expr, parse_for_parenthesized_expression(ctx, "END"), "END");
            }
            else if (skip_keyword_if_is(pos, end, "LIMIT"))
            {
                assign_for_clause_once(output.limit_expr, parse_for_parenthesized_expression(ctx, "LIMIT"), "LIMIT");
            }
            else if (skip_keyword_if_is(pos, end, "FILTER"))
            {
                assign_for_clause_once(output.filter_expr, parse_for_parenthesized_expression(ctx, "FILTER"), "FILTER");
            }
            else if (skip_keyword_if_is(pos, end, "BY"))
            {
                assign_for_clause_once(output.by_expr, parse_for_parenthesized_expression(ctx, "BY"), "BY");
            }
            else if (skip_keyword_if_is(pos, end, "FROM"))
            {
                assign_for_clause_once(output.from_expr, parse_for_parenthesized_expression(ctx, "FROM"), "FROM");
            }
            else if (skip_keyword_if_is(pos, end, "TO"))
            {
                assign_for_clause_once(output.to_expr, parse_for_parenthesized_expression(ctx, "TO"), "TO");
            }
            else if (skip_keyword_if_is(pos, end, "UNTIL"))
            {
                assign_for_clause_once(output.until_expr, parse_for_parenthesized_expression(ctx, "UNTIL"), "UNTIL");
            }
            else
            {
                throw syntax_compilation_error("Expected FOR clause or LOOP");
            }
        }
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_FOR_STATEMENT_HEADER_GUARD
