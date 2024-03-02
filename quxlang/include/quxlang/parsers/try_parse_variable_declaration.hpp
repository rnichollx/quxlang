//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_VARIABLE_DECLARATION_HPP
#define TRY_PARSE_VARIABLE_DECLARATION_HPP

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_variable_declaration > try_parse_variable_declaration(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        std::optional< ast2_variable_declaration > output;

        if (!skip_keyword_if_is(pos, end, "VAR"))
        {
            return output;
        }

        skip_wsc(pos, end);

        type_symbol type = try_parse_type_symbol(pos, end).value();

        skip_wsc(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::runtime_error("Expected ';' after VAR type");
        }

        output = ast2_variable_declaration{};

        output->type = type;

        return output;
    }
} // namespace quxlang

#endif //TRY_PARSE_VARIABLE_DECLARATION_HPP
