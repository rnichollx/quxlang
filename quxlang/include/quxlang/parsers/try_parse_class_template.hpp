//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef QUXLANG_TRY_PARSE_CLASS_TEMPLATE_HPP
#define QUXLANG_TRY_PARSE_CLASS_TEMPLATE_HPP

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_class.hpp>


namespace quxlang::parsers
{
    template < typename It >
    std::optional< quxlang::ast2_template_declaration > try_parse_template(It& pos, It end)
    {
        auto pos2 = pos;


        if (!skip_keyword_if_is(pos2, end, "TEMPLATE"))
        {
            return std::nullopt;
        }
        pos = pos2;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '(' after CLASS TEMPLATE");
        }
        std::optional< quxlang::ast2_template_declaration > ct = ast2_template_declaration{};
    get_arg:
        skip_whitespace_and_comments(pos, end);

        auto arg = parse_type_symbol(pos, end);

        ct->m_template_args.push_back(arg);
        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ","))
        {
            goto get_arg;
        }
        else if (skip_symbol_if_is(pos, end, ")"))
        {
            skip_whitespace_and_comments(pos, end);
            // TODO: Also allow template functions.

            ast2_class_declaration class_body = parse_class(pos, end);

            ct->m_class = class_body;
            return ct;
        }
        else
        {
            throw std::logic_error("Expected ',' or ')' after CLASS TEMPLATE(...");
        }
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_CLASS_TEMPLATE_HPP
