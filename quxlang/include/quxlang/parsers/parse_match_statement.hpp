// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_MATCH_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_MATCH_STATEMENT_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/fwd.hpp>
#include <quxlang/parsers/parse_bounded_statement_expression.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_subentity.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace quxlang::parsers
{

    /** Parses a statement-only MATCH over a UNION or VARIANT expression. */
    inline auto parse_match_statement(parsing_context& ctx) -> function_match_statement
    {
        auto& position = ctx.iter_pos;
        parse_iterator const end = ctx.iter_end;
        parse_iterator const statement_begin = position;

        skip_whitespace_and_comments(position, end);
        if (!skip_keyword_if_is(position, end, "MATCH"))
        {
            throw syntax_compilation_error("Expected MATCH");
        }
        skip_whitespace_and_comments(position, end);

        parse_iterator const subject_begin = position;
        detail::statement_expression_boundary const boundary = detail::find_statement_expression_boundary(
            position, end, false, true, "Expected AS, SHADOW, or '{' after MATCH expression");

        function_match_statement result;
        result.subject = detail::parse_bounded_statement_expression(ctx, subject_begin, boundary.position, "Unexpected text in MATCH subject expression");
        position = boundary.position;

        if (boundary.delimiter == detail::statement_expression_delimiter::as_binding)
        {
            if (!skip_keyword_if_is(position, end, "AS"))
            {
                throw compiler_bug("MATCH AS boundary did not point at AS");
            }
            skip_whitespace_and_comments(position, end);
            std::string binding_name = parse_identifier(position, end);
            if (binding_name.empty())
            {
                throw syntax_compilation_error("Expected binding name after MATCH expression AS");
            }
            result.binding_name = std::move(binding_name);
        }
        else if (boundary.delimiter == detail::statement_expression_delimiter::shadow_binding)
        {
            if (!skip_keyword_if_is(position, end, "SHADOW"))
            {
                throw compiler_bug("MATCH SHADOW boundary did not point at SHADOW");
            }
            std::optional< std::string > const bare_identifier = detail::parse_bounded_statement_bare_identifier(ctx, subject_begin, boundary.position);
            if (!bare_identifier.has_value())
            {
                throw syntax_compilation_error("MATCH SHADOW requires a bare identifier subject");
            }
            result.shadow = true;
            result.binding_name = *bare_identifier;
        }

        skip_whitespace_and_comments(position, end);
        if (!skip_symbol_if_is(position, end, "{"))
        {
            throw syntax_compilation_error("Expected '{' after MATCH header");
        }

        while (true)
        {
            skip_whitespace_and_comments(position, end);
            if (skip_symbol_if_is(position, end, "}"))
            {
                result.location = ctx.get_location_optional(statement_begin, position);
                return result;
            }

            parse_iterator const clause_begin = position;
            if (skip_keyword_if_is(position, end, "DEFAULT"))
            {
                if (result.default_clause.has_value())
                {
                    throw syntax_compilation_error("MATCH may contain only one DEFAULT clause");
                }

                function_match_default default_clause;
                skip_whitespace_and_comments(position, end);
                if (skip_keyword_if_is(position, end, "FAIL"))
                {
                    default_clause.fail = true;
                    skip_whitespace_and_comments(position, end);
                    if (!skip_symbol_if_is(position, end, ";"))
                    {
                        throw syntax_compilation_error("Expected ';' after DEFAULT FAIL");
                    }
                }
                else
                {
                    default_clause.block = parse_function_block(ctx);
                }
                default_clause.location = ctx.get_location_optional(clause_begin, position);
                result.default_clause = std::move(default_clause);

                skip_whitespace_and_comments(position, end);
                if (!skip_symbol_if_is(position, end, "}"))
                {
                    throw syntax_compilation_error("DEFAULT must be the final MATCH clause");
                }
                result.location = ctx.get_location_optional(statement_begin, position);
                return result;
            }

            function_match_arm arm;
            if (skip_keyword_if_is(position, end, "CASE"))
            {
                skip_whitespace_and_comments(position, end);
                union_match_selector selector;
                selector.option_name = parse_subentity(position, end);
                if (selector.option_name.empty())
                {
                    throw syntax_compilation_error("Expected UNION option name after CASE");
                }
                selector.location = ctx.get_location_optional(clause_begin, position);
                arm.selector = std::move(selector);
            }
            else if (skip_keyword_if_is(position, end, "TYPE"))
            {
                skip_whitespace_and_comments(position, end);
                variant_match_selector selector;
                std::optional< type_symbol > const parsed_type = try_parse_type_symbol(ctx);
                if (!parsed_type.has_value())
                {
                    throw syntax_compilation_error("Expected VARIANT alternative type after TYPE");
                }
                selector.type = *parsed_type;
                selector.location = ctx.get_location_optional(clause_begin, position);
                arm.selector = std::move(selector);
            }
            else
            {
                throw syntax_compilation_error("Expected CASE, TYPE, DEFAULT, or '}' in MATCH");
            }

            skip_whitespace_and_comments(position, end);
            if (skip_keyword_if_is(position, end, "AS"))
            {
                skip_whitespace_and_comments(position, end);
                std::string binding_name = parse_identifier(position, end);
                if (binding_name.empty())
                {
                    throw syntax_compilation_error("Expected binding name after MATCH arm AS");
                }
                arm.binding_name = std::move(binding_name);
                skip_whitespace_and_comments(position, end);
            }

            if (skip_keyword_if_is(position, end, "WHERE"))
            {
                skip_whitespace_and_comments(position, end);
                arm.where_condition = parse_expression(ctx);
            }
            else if (skip_keyword_if_is(position, end, "OTHERWISE"))
            {
                arm.otherwise = true;
            }

            skip_whitespace_and_comments(position, end);
            arm.block = parse_function_block(ctx);
            arm.location = ctx.get_location_optional(clause_begin, position);
            result.arms.push_back(std::move(arm));
        }
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_MATCH_STATEMENT_HEADER_GUARD
