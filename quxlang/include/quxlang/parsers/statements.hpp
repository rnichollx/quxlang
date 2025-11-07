// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com


#ifndef QUXLANG_PARSERS_STATEMENTS_HEADER_GUARD
#define QUXLANG_PARSERS_STATEMENTS_HEADER_GUARD

#include <quxlang/data/statements.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional<function_place_statement> try_parse_place_statement(It& pos, It end)
    {
        // there are three forms,
        // PLACE AT(loc) type :(args...);
        // PLACE AT(loc) type := assign_init_expr;
        // PLACE AT(loc) type;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "PLACE"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "AT"))
        {
            throw std::logic_error("Expected 'AT' after 'PLACE'");
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '(' after PLACE AT");
        }

        skip_whitespace_and_comments(pos, end);
        function_place_statement result;
        result.at = parse_expression(pos, end);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::logic_error("Expected ')' after PLACE AT(location expression)");
        }

        skip_whitespace_and_comments(pos, end);
        result.type = parse_type_symbol(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ":("))
        {
            // constructor argument form
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                while (true)
                {
                    skip_whitespace_and_comments(pos, end);
                    expression_arg arg = parse_expression_arg(pos, end);
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
                        throw std::logic_error("Expected ',' or ')' in PLACE args");
                    }
                }
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ';' after PLACE statement");
            }
            return std::optional<function_place_statement>{std::move(result)};
        }
        else if (skip_symbol_if_is(pos, end, ":="))
        {
            // assignment initializer form
            skip_whitespace_and_comments(pos, end);
            result.assign_init = parse_expression(pos, end);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ';' after PLACE := initializer");
            }
            return result;
        }
        else
        {
            // bare form
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ';' after PLACE statement");
            }
            return result;
        }
    }

    template < typename It >
    function_place_statement parse_place_statement(It& pos, It end)
    {
        auto res = try_parse_place_statement(pos, end);
        if (!res)
        {
            throw std::logic_error("Expected PLACE statement");
        }
        return std::move(*res);
    }

    template < typename It >
    auto parse_destroy_statement(It& pos, It end) -> function_destroy_statement
    {
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "DESTROY"))
        {
            throw std::logic_error("Expected 'DESTROY'");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "AT"))
        {
            throw std::logic_error("Expected 'AT' after 'DESTROY'");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '(' after DESTROY AT");
        }
        skip_whitespace_and_comments(pos, end);
        function_destroy_statement result;
        result.at = parse_expression(pos, end);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::logic_error("Expected ')' after DESTROY AT(location expression)");
        }
        skip_whitespace_and_comments(pos, end);
        result.type = parse_type_symbol(pos, end);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ';' after DESTROY statement");
        }
        return result;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_STATEMENT_HPP
