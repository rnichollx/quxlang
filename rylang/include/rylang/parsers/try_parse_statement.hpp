//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_STATEMENT_HPP
#define TRY_PARSE_STATEMENT_HPP
#include <rylang/data/function_statement.hpp>
#include <rylang/parsers/parse_whitespace_and_comments.hpp>
#include <rylang/parsers/try_parse_expression_statement.hpp>
#include <rylang/parsers/parse_if_statement.hpp>
#include <rylang/parsers/parse_var_statement.hpp>
#include <rylang/parsers/parse_return_statement.hpp>
#include <rylang/parsers/parse_while_statement.hpp>


namespace rylang::parsers
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

        std::optional< function_expression_statement > exp_st;

        if (get_keyword(pos, end) == "IF")
        {
            return parse_if_statement(pos, end);
        }
        else if (get_keyword(pos, end) == "VAR")
        {
            return parse_var_statement(pos, end);
        }
        else if (get_keyword(pos, end) == "RETURN")
        {
            return parse_return_statement(pos, end);
        }
        else if (get_keyword(pos, end) == "WHILE")
        {
            return parse_while_statement(pos, end);
        }
        else if (auto expr_st = try_parse_expression_statement(pos, end); expr_st)
        {
            return *expr_st;
        }

        return std::nullopt;
    }
} // namespace rylang::parsers

#endif // TRY_PARSE_STATEMENT_HPP
