//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_DECLARATION_HPP
#define TRY_PARSE_DECLARATION_HPP
#include <rylang/ast2/ast2_entity.hpp>
#include <rylang/parsers/try_parse_class.hpp>
#include <rylang/parsers/try_parse_class_template.hpp>
#include <rylang/parsers/try_parse_function_declaration.hpp>
#include <rylang/parsers/try_parse_variable_declaration.hpp>
#include <rylang/parsers/try_parse_namespace.hpp>


namespace rylang::parsers
{
    template < typename It >
    std::optional< ast2_class_declaration > try_parse_class(It& pos, It end);

    template < typename It >
    std::optional< ast2_declarable > try_parse_declarable(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        std::optional< ast2_declarable > output;
        output = try_parse_class(pos, end);
        if (output)
        {
            return output;
        }
        output = try_parse_function_declaration(pos, end);
        if (output)
        {
            return output;
        }
        output = try_parse_class_template(pos, end);
        if (output)
        {
            return output;
        }
        output = try_parse_variable_declaration(pos, end);

        if (output)
        {
            return output;
        }

        output = try_parse_namespace(pos, end);
        return output;
    }
} // namespace rylang::parsers

#endif // TRY_PARSE_DECLARATION_HPP
