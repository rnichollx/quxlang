// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_VISIT_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_VISIT_STATEMENT_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/fwd.hpp>
#include <quxlang/parsers/parse_bounded_statement_expression.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

#include <optional>
#include <string>

namespace quxlang::parsers
{
    /** Parses one of the five VISIT statement forms. */
    inline auto parse_visit_statement(parsing_context& ctx) -> function_visit_statement
    {
        parse_iterator& position = ctx.iter_pos;
        parse_iterator const end = ctx.iter_end;
        parse_iterator const statement_begin = position;

        skip_whitespace_and_comments(position, end);
        if (!skip_keyword_if_is(position, end, "VISIT"))
        {
            throw syntax_compilation_error("Expected VISIT");
        }
        skip_whitespace_and_comments(position, end);

        bool const extend_temporaries = skip_keyword_if_is(position, end, "EXTEND");
        if (extend_temporaries)
        {
            skip_whitespace_and_comments(position, end);
        }

        parse_iterator const subject_begin = position;
        detail::statement_expression_boundary const boundary = detail::find_statement_expression_boundary(
            position, end, true, false, "Expected AS, ';', or '{' after VISIT expression");

        function_visit_statement result;
        result.subject = detail::parse_bounded_statement_expression(ctx, subject_begin, boundary.position, "Unexpected text in VISIT subject expression");
        position = boundary.position;

        if (boundary.delimiter == detail::statement_expression_delimiter::as_binding)
        {
            if (!skip_keyword_if_is(position, end, "AS"))
            {
                throw compiler_bug("VISIT AS boundary did not point at AS");
            }
            skip_whitespace_and_comments(position, end);
            result.binding_name = parse_identifier(position, end);
            if (result.binding_name.empty())
            {
                throw syntax_compilation_error("Expected binding name after VISIT expression AS");
            }
            skip_whitespace_and_comments(position, end);

            if (skip_symbol_if_is(position, end, ";"))
            {
                result.form = extend_temporaries ? function_visit_form::extended_named_continuation : function_visit_form::named_continuation;
            }
            else
            {
                if (extend_temporaries)
                {
                    throw syntax_compilation_error("VISIT EXTEND requires the continuation form ending in ';'");
                }
                result.form = function_visit_form::named_block;
                result.body = parse_function_block(ctx);
            }
        }
        else
        {
            if (extend_temporaries)
            {
                throw syntax_compilation_error("VISIT EXTEND requires an AS binding");
            }
            std::optional< std::string > const bare_identifier = detail::parse_bounded_statement_bare_identifier(ctx, subject_begin, boundary.position);
            if (!bare_identifier.has_value())
            {
                throw syntax_compilation_error("VISIT without AS requires a bare identifier subject");
            }
            result.binding_name = *bare_identifier;

            if (boundary.delimiter == detail::statement_expression_delimiter::terminator)
            {
                if (!skip_symbol_if_is(position, end, ";"))
                {
                    throw compiler_bug("VISIT terminator boundary did not point at ';'");
                }
                result.form = function_visit_form::variable_continuation;
            }
            else
            {
                result.form = function_visit_form::variable_block;
                result.body = parse_function_block(ctx);
            }
        }

        result.location = ctx.get_location_optional(statement_begin, position);
        return result;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_PARSE_VISIT_STATEMENT_HEADER_GUARD
