// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_BOUNDED_STATEMENT_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_BOUNDED_STATEMENT_EXPRESSION_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

#include <cstddef>
#include <optional>
#include <string>

namespace quxlang::parsers::detail
{
    /** Identifies the top-level token that ends a statement-header expression. */
    enum class statement_expression_delimiter
    {
        body,
        terminator,
        as_binding,
        shadow_binding,
    };

    /** Records where and how a nesting-aware statement-header expression ends. */
    struct statement_expression_boundary
    {
        parse_iterator position;
        statement_expression_delimiter delimiter;
    };

    /** Finds a top-level statement-header delimiter while skipping nested expressions and comments. */
    inline auto find_statement_expression_boundary(parse_iterator position, parse_iterator end, bool permit_terminator, bool permit_shadow,
                                                    std::string const& missing_delimiter_message) -> statement_expression_boundary
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
                    return statement_expression_boundary{.position = position, .delimiter = statement_expression_delimiter::body};
                }
                if (permit_terminator && *position == ';')
                {
                    return statement_expression_boundary{.position = position, .delimiter = statement_expression_delimiter::terminator};
                }
                if (separated_from_expression && !top_level_lambda_body_pending)
                {
                    parse_iterator trial = position;
                    if (permit_shadow && skip_keyword_if_is(trial, end, "SHADOW"))
                    {
                        return statement_expression_boundary{.position = position, .delimiter = statement_expression_delimiter::shadow_binding};
                    }
                    trial = position;
                    if (skip_keyword_if_is(trial, end, "AS"))
                    {
                        return statement_expression_boundary{.position = position, .delimiter = statement_expression_delimiter::as_binding};
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

        throw syntax_compilation_error(missing_delimiter_message);
    }

    /** Parses an expression whose input is explicitly bounded by its statement-header delimiter. */
    inline auto parse_bounded_statement_expression(parsing_context const& outer_context, parse_iterator begin, parse_iterator end,
                                                   std::string const& unexpected_text_message) -> expression
    {
        parsing_context subject_context = outer_context;
        subject_context.iter_pos = begin;
        subject_context.iter_end = end;
        expression subject = parse_expression(subject_context);
        skip_whitespace_and_comments(subject_context.iter_pos, subject_context.iter_end);
        if (subject_context.iter_pos != subject_context.iter_end)
        {
            throw syntax_compilation_error(unexpected_text_message);
        }
        return subject;
    }

    /** Returns the identifier when a bounded statement-header expression is syntactically one bare identifier. */
    inline auto parse_bounded_statement_bare_identifier(parsing_context const& outer_context, parse_iterator begin, parse_iterator end)
        -> std::optional< std::string >
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
} // namespace quxlang::parsers::detail

#endif // QUXLANG_PARSERS_PARSE_BOUNDED_STATEMENT_EXPRESSION_HEADER_GUARD
