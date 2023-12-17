//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_VAR_STATEMENT_HPP
#define PARSE_VAR_STATEMENT_HPP
#include <rylang/data/function_statement.hpp>
#include <rylang/parsers/parse_type_symbol.hpp>

namespace rylang::parsers
{
    template < typename It >
    function_var_statement parse_var_statement(It& pos, It end)
    {
        skip_wsc(pos, end);

        if (!skip_keyword_if_is(pos, end, "VAR"))
        {
            throw std::runtime_error("Expected 'VAR'");
        }

        skip_wsc(pos, end);

        function_var_statement var_statement;

        var_statement.name = get_skip_identifier(pos, end);

        skip_wsc(pos, end);

        std::string remaining{pos, end};
        var_statement.type = parse_type_symbol(pos, end);

        skip_wsc(pos, end);

        if (skip_symbol_if_is(pos, end, ":("))
        {

            while (true)
            {

                skip_wsc(pos, end);
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
                    throw std::runtime_error("Expected ',' or ')'");
                }
            }
        }

        std::string remaining2{pos, end};

        skip_wsc(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::runtime_error("Expected ';'");
        }

        return var_statement;
    }
} // namespace rylang

#endif // PARSE_VAR_STATEMENT_HPP
