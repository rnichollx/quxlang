// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_MATCH_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_MATCH_STATEMENT_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/fwd.hpp>
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
    namespace detail
    {
        enum class match_header_delimiter
        {
            body,
            as_binding,
            shadow_binding,
        };

        /** Records which optional MATCH header clause bounded the parsed subject. */
        struct match_header_boundary
        {
            parse_iterator position;
            match_header_delimiter delimiter;
        };

        /** Finds a MATCH header delimiter without treating delimiters nested in expressions as clauses. */
        inline auto find_match_header_boundary(parse_iterator position, parse_iterator end) -> match_header_boundary
        {
            std::size_t parenthesis_depth = 0;
            std::size_t bracket_depth = 0;
            std::size_t brace_depth = 0;
            bool separated_from_expression = false;
            bool top_level_lambda_body_pending = false;

            while (position != end)
            {
                bool const at_top_level = parenthesis_depth == 0 && bracket_depth == 0 && brace_depth == 0;
                if (*position == ' ' || *position == '\t' || *position == '\r' || *position == '\n' || *position == '\f' || *position == '\v')
                {
                    if (at_top_level)
                    {
                        separated_from_expression = true;
                    }
                    ++position;
                    continue;
                }

                if (*position == '"' || *position == '\'')
                {
                    if (at_top_level)
                    {
                        separated_from_expression = true;
                    }
                    char const quote = *position++;
                    bool escaped = false;
                    while (position != end)
                    {
                        char const character = *position++;
                        if (escaped)
                        {
                            escaped = false;
                        }
                        else if (character == '\\')
                        {
                            escaped = true;
                        }
                        else if (character == quote)
                        {
                            break;
                        }
                    }
                    continue;
                }

                parse_iterator next = position;
                ++next;
                if (*position == '/' && next != end && *next == '/')
                {
                    if (at_top_level)
                    {
                        separated_from_expression = true;
                    }
                    position = ++next;
                    while (position != end && *position != '\n')
                    {
                        ++position;
                    }
                    continue;
                }
                if (*position == '/' && next != end && *next == '*')
                {
                    if (at_top_level)
                    {
                        separated_from_expression = true;
                    }
                    position = ++next;
                    while (position != end)
                    {
                        parse_iterator comment_next = position;
                        ++comment_next;
                        if (*position == '*' && comment_next != end && *comment_next == '/')
                        {
                            position = ++comment_next;
                            break;
                        }
                        ++position;
                    }
                    continue;
                }

                if (at_top_level)
                {
                    if (*position == '{')
                    {
                        if (top_level_lambda_body_pending)
                        {
                            top_level_lambda_body_pending = false;
                            ++brace_depth;
                            separated_from_expression = false;
                            ++position;
                            continue;
                        }
                        return match_header_boundary{.position = position, .delimiter = match_header_delimiter::body};
                    }
                    if (separated_from_expression && !top_level_lambda_body_pending)
                    {
                        parse_iterator trial = position;
                        if (skip_keyword_if_is(trial, end, "SHADOW"))
                        {
                            return match_header_boundary{.position = position, .delimiter = match_header_delimiter::shadow_binding};
                        }
                        trial = position;
                        if (skip_keyword_if_is(trial, end, "AS"))
                        {
                            return match_header_boundary{.position = position, .delimiter = match_header_delimiter::as_binding};
                        }
                    }
                }

                if (at_top_level && *position == '-' && next != end && *next == '<')
                {
                    top_level_lambda_body_pending = true;
                }
                else if (at_top_level && top_level_lambda_body_pending && *position == '=')
                {
                    top_level_lambda_body_pending = false;
                }

                if (*position == '(')
                {
                    ++parenthesis_depth;
                }
                else if (*position == ')' && parenthesis_depth != 0)
                {
                    --parenthesis_depth;
                }
                else if (*position == '[')
                {
                    ++bracket_depth;
                }
                else if (*position == ']' && bracket_depth != 0)
                {
                    --bracket_depth;
                }
                else if (*position == '{')
                {
                    ++brace_depth;
                }
                else if (*position == '}' && brace_depth != 0)
                {
                    --brace_depth;
                }
                bool const returned_to_top_level = !at_top_level && parenthesis_depth == 0 && bracket_depth == 0 && brace_depth == 0;
                if (returned_to_top_level)
                {
                    separated_from_expression = *position == ')' || *position == ']' || *position == '}';
                }
                else if (at_top_level)
                {
                    separated_from_expression = false;
                }
                ++position;
            }

            throw syntax_compilation_error("Expected AS, SHADOW, or '{' after MATCH expression");
        }

        /** Parses an expression whose input is explicitly bounded by the MATCH header delimiter. */
        inline auto parse_bounded_match_subject(parsing_context const& outer_context, parse_iterator begin, parse_iterator end) -> expression
        {
            parsing_context subject_context = outer_context;
            subject_context.iter_pos = begin;
            subject_context.iter_end = end;
            expression subject = parse_expression(subject_context);
            skip_whitespace_and_comments(subject_context.iter_pos, subject_context.iter_end);
            if (subject_context.iter_pos != subject_context.iter_end)
            {
                throw syntax_compilation_error("Unexpected text in MATCH subject expression");
            }
            return subject;
        }

        /** Returns the identifier when a bounded MATCH subject is syntactically one bare identifier. */
        inline auto parse_bounded_match_bare_identifier(parsing_context const& outer_context, parse_iterator begin, parse_iterator end) -> std::optional< std::string >
        {
            parsing_context identifier_context = outer_context;
            identifier_context.iter_pos = begin;
            identifier_context.iter_end = end;
            skip_whitespace_and_comments(identifier_context.iter_pos, identifier_context.iter_end);
            std::string identifier = parse_identifier(identifier_context.iter_pos, identifier_context.iter_end);
            if (identifier.empty())
            {
                return std::nullopt;
            }
            skip_whitespace_and_comments(identifier_context.iter_pos, identifier_context.iter_end);
            if (identifier_context.iter_pos != identifier_context.iter_end)
            {
                return std::nullopt;
            }
            return identifier;
        }
    } // namespace detail

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
        detail::match_header_boundary const boundary = detail::find_match_header_boundary(position, end);

        function_match_statement result;
        result.subject = detail::parse_bounded_match_subject(ctx, subject_begin, boundary.position);
        position = boundary.position;

        if (boundary.delimiter == detail::match_header_delimiter::as_binding)
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
        else if (boundary.delimiter == detail::match_header_delimiter::shadow_binding)
        {
            if (!skip_keyword_if_is(position, end, "SHADOW"))
            {
                throw compiler_bug("MATCH SHADOW boundary did not point at SHADOW");
            }
            std::optional< std::string > const bare_identifier = detail::parse_bounded_match_bare_identifier(ctx, subject_begin, boundary.position);
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
