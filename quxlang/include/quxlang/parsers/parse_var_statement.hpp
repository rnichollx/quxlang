// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>

namespace quxlang::parsers
{
    template < typename It >
    function_var_statement parse_var_statement(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "VAR"))
        {
            throw std::logic_error("Expected 'VAR'");
        }

        skip_whitespace_and_comments(pos, end);

        function_var_statement var_statement;

        var_statement.name = parse_identifier(pos, end);

        skip_whitespace_and_comments(pos, end);

        std::string remaining{pos, end};
        var_statement.type = parse_type_symbol(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ":("))
        {

            while (true)
            {

                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    break;
                }

                remaining = std::string(pos, end);

                expression expr = parse_expression(pos, end);
                var_statement.initializers.push_back(std::move(expr));

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
                    throw std::logic_error("Expected ',' or ')'");
                }
            }
        }

        std::string remaining2{pos, end};

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ';'");
        }

        return var_statement;
    }
} // namespace quxlang

#endif // PARSE_VAR_STATEMENT_HPP
