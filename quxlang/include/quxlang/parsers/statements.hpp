// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com


#ifndef QUXLANG_PARSERS_STATEMENTS_HEADER_GUARD
#define QUXLANG_PARSERS_STATEMENTS_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <quxlang/data/statements.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>

#include <utility>

namespace quxlang::parsers
{
    inline std::optional<function_place_statement> try_parse_place_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "PLACE"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "AT"))
        {
            throw syntax_compilation_error("Expected 'AT' after 'PLACE'");
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after PLACE AT");
        }

        skip_whitespace_and_comments(pos, end);
        function_place_statement result;
        result.at = parse_expression(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after PLACE AT(location expression)");
        }

        skip_whitespace_and_comments(pos, end);
        result.type = parse_type_symbol(ctx);

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ":("))
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                while (true)
                {
                    skip_whitespace_and_comments(pos, end);
                    expression_arg arg = parse_expression_arg(ctx);
                    result.args.push_back(std::move(arg));
                    skip_whitespace_and_comments(pos, end);
                    if (skip_symbol_if_is(pos, end, ","))
                    {
                        continue;
                    }
                    else if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }
                    else
                    {
                        throw syntax_compilation_error("Expected ',' or ')' in PLACE args");
                    }
                }
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after PLACE statement");
            }
            result.location = ctx.get_location_optional(begin, pos);
            return std::optional<function_place_statement>{std::move(result)};
        }
        else if (skip_symbol_if_is(pos, end, ":="))
        {
            skip_whitespace_and_comments(pos, end);
            result.assign_init = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after PLACE := initializer");
            }
            result.location = ctx.get_location_optional(begin, pos);
            return std::move(result);
        }
        else
        {
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after PLACE statement");
            }
            result.location = ctx.get_location_optional(begin, pos);
            return std::move(result);
        }
    }

    inline function_place_statement parse_place_statement(parsing_context& ctx)
    {
        auto res = try_parse_place_statement(ctx);
        if (!res)
        {
            throw syntax_compilation_error("Expected PLACE statement");
        }
        return std::move(*res);
    }

    inline auto parse_destroy_statement(parsing_context& ctx) -> function_destroy_statement
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "DESTROY"))
        {
            throw syntax_compilation_error("Expected 'DESTROY'");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "AT"))
        {
            throw syntax_compilation_error("Expected 'AT' after 'DESTROY'");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after DESTROY AT");
        }
        skip_whitespace_and_comments(pos, end);
        function_destroy_statement result;
        result.at = parse_expression(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after DESTROY AT(location expression)");
        }
        skip_whitespace_and_comments(pos, end);
        result.type = parse_type_symbol(ctx);
        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, ":("))
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                while (true)
                {
                    skip_whitespace_and_comments(pos, end);
                    result.args.push_back(parse_expression_arg(ctx));
                    skip_whitespace_and_comments(pos, end);
                    if (skip_symbol_if_is(pos, end, ","))
                    {
                        continue;
                    }
                    else if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }
                    else
                    {
                        throw syntax_compilation_error("Expected ',' or ')' in DESTROY args");
                    }
                }
            }
            skip_whitespace_and_comments(pos, end);
        }
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after DESTROY statement");
        }
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }

} // namespace quxlang::parsers

#endif // QUXLANG_STATEMENT_HPP
