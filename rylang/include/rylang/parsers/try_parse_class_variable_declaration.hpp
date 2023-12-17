//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_CLASS_VARIABLE_DECLARATION_HPP
#define TRY_PARSE_CLASS_VARIABLE_DECLARATION_HPP
#include <rylang/ast2/ast2_named_variable_declaration.hpp>
#include <rylang/parsers/skip_keyword_if_is.hpp>
#include <rylang/parsers/skip_symbol_if_is.hpp>
#include <rylang/parsers/try_parse_type_symbol.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::optional< ast2_named_variable_declaration > try_parse_class_variable_declaration(It& pos, It end)
    {
        std::optional< ast2_named_variable_declaration > out;

        auto pos2 = pos;

        bool is_member;

        if (skip_symbol_if_is(pos2, end, "."))
        {
            is_member = true;
        }
        else if (skip_symbol_if_is(pos2, end, "::"))
        {
            is_member = false;
        }
        else
        {
            return out;
        }
        std::string name = get_skip_identifier(pos2, end);

        if (name.empty())
        {
            throw std::runtime_error("Expected identifier");
        }

        skip_whitespace(pos2, end);

        if (!skip_keyword_if_is(pos2, end, "VAR"))
        {
            return out;
        }

        skip_wsc(pos2, end);

        type_symbol type = try_parse_type_symbol(pos, end).value();

        skip_wsc(pos2, end);

        if (!skip_symbol_if_is(pos2, end, ";"))
        {
            throw std::runtime_error("Expected ';' after VAR type");
        }

        pos = pos2;

        out = ast2_named_variable_declaration{};
        out->name = name;
        out->type = type;
        out->is_member = is_member;

        return out;
    }
} // namespace rylang::parsers

#endif // TRY_PARSE_CLASS_VARIABLE_DECLARATION_HPP
