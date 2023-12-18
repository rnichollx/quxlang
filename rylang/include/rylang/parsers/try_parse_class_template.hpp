//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef RYLANG_TRY_PARSE_CLASS_TEMPLATE_HPP
#define RYLANG_TRY_PARSE_CLASS_TEMPLATE_HPP

#include <rylang/ast2/ast2_entity.hpp>
#include <rylang/parsers/parse_class.hpp>


namespace rylang::parsers
{
    template < typename It >
    std::optional< rylang::ast2_class_template_declaration > try_parse_class_template(It& pos, It end)
    {
        auto pos2 = pos;

        skip_wsc(pos2, end);
        if (!skip_keyword_if_is(pos2, end, "CLASS"))
        {
            return std::nullopt;
        }
        skip_wsc(pos2, end);
        if (!skip_keyword_if_is(pos2, end, "TEMPLATE"))
        {
            return std::nullopt;
        }
        pos = pos2;
        skip_wsc(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::runtime_error("Expected '(' after CLASS TEMPLATE");
        }
        std::optional< rylang::ast2_class_template_declaration > ct = ast2_class_template_declaration{};
    get_arg:
        skip_wsc(pos, end);

        auto arg = parse_type_symbol(pos, end);

        ct->m_template_args.push_back(arg);
        skip_wsc(pos, end);

        if (skip_symbol_if_is(pos, end, ","))
        {
            goto get_arg;
        }
        else if (skip_symbol_if_is(pos, end, ")"))
        {
            skip_wsc(pos, end);
            if (!skip_symbol_if_is(pos, end, "{"))
            {
                throw std::runtime_error("Expected '{' after CLASS TEMPLATE(...)");
            }

            ast2_class_declaration class_body = parse_class(pos, end);

            return ct;
        }
        else
        {
            throw std::runtime_error("Expected ',' or ')' after CLASS TEMPLATE(...");
        }
    }

} // namespace rylang::parsers

#endif // TRY_PARSE_CLASS_TEMPLATE_HPP
