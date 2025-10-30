// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_TRY_PARSE_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_STATEMENT_HEADER_GUARD
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_if_statement.hpp>
#include <quxlang/parsers/parse_return_statement.hpp>
#include <quxlang/parsers/parse_var_statement.hpp>
#include <quxlang/parsers/parse_while_statement.hpp>
#include <quxlang/parsers/statements.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_expression_statement.hpp>
#include <quxlang/parsers/fwd.hpp> // added forward declarations

namespace quxlang::parsers
{
    template < typename It >
    std::optional< function_statement > try_parse_statement(It& pos, It end)
    {
        std::optional< function_statement > output;
        skip_whitespace_and_comments(pos, end);

        std::string remaining = std::string(pos, end);

        if (remaining.starts_with("I32::CONSTRU"))
        {
            int x = 0;
        }

        if (auto res = try_parse_function_block(pos, end); res)
        {
            return *res;
        }

        std::optional< function_expression_statement > exp_st;

        if (skip_keyword_if_is(pos, end, "UNIMPLEMENTED"))
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ';' after UNIMPLEMENTED statement");
            }
            // TODO: Support error message string.
            return function_unimplemented_statement{};
        }
        else if (next_keyword(pos, end) == "PLACE")
        {
            return parse_place_statement(pos, end);
        }
        else if (next_keyword(pos, end) == "DESTROY")
        {
            return parse_destroy_statement(pos, end);
        }
        else if (next_keyword(pos, end) == "IF")
        {
            return parse_if_statement(pos, end);
        }
        else if (next_keyword(pos, end) == "VAR")
        {
            return parse_var_statement(pos, end);
        }
        else if (next_keyword(pos, end) == "RETURN")
        {
            return parse_return_statement(pos, end);
        }
        else if (next_keyword(pos, end) == "WHILE")
        {
            return parse_while_statement(pos, end);
        }
        if (next_keyword(pos, end) == "ASSERT")
        {
            return parse_assert_statement(pos, end);
        }
        else if (auto expr_st = try_parse_expression_statement(pos, end); expr_st)
        {
            return *expr_st;
        }

        return std::nullopt;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_STATEMENT_HPP
