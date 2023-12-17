//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
#define TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
#include <rylang/ast2/ast2_named_function_declaration.hpp>
#include <rylang/parsers/parse_function_block.hpp>
#include <rylang/parsers/skip_keyword_if_is.hpp>
#include <rylang/parsers/skip_symbol_if_is.hpp>

#include <rylang/parsers/try_parse_function_declaration.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::optional<ast2_named_function_declaration> try_parse_class_function_declaration(It& pos, It end)
    {
        std::optional<ast2_named_function_declaration> out;

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

        if (!skip_keyword_if_is(pos2, end, "FUNCTION"))
        {
            return out;
        }

        skip_wsc(pos2, end);

        auto function_opt  = try_parse_function_declaration(pos2, end);
        if (!function_opt)
        {
            return out;
        }

        auto function = *function_opt;

        pos = pos2;

        out = ast2_named_function_declaration{};
        out->name = name;
        out->is_field = is_member;
        out->function = function;

        return out;
    }

}

#endif //TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
