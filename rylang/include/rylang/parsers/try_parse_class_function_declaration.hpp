//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
#define TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
#include <rylang/parsers/parse_function_block.hpp>
#include <rylang/parsers/skip_keyword_if_is.hpp>
#include <rylang/parsers/skip_symbol_if_is.hpp>

#include <rylang/parsers/try_parse_function_declaration.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::optional< std::tuple< std::string, bool, ast2_function_declaration > > try_parse_class_function_declaration(It& pos, It end)
    {
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
            return std::nullopt;
        }
        std::string name = get_skip_identifier(pos2, end);

        if (name.empty())
        {
            throw std::runtime_error("Expected identifier");
        }

        skip_whitespace_and_comments(pos2, end);

        if (!skip_keyword_if_is(pos2, end, "FUNCTION"))
        {
            return std::nullopt;
        }

        skip_wsc(pos2, end);

        auto function_opt = try_parse_function_declaration(pos2, end);
        if (!function_opt)
        {
            return std::nullopt;
        }

        auto function = *function_opt;

        pos = pos2;

        return { { name, is_member, function } };
    }

} // namespace rylang::parsers

#endif // TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
