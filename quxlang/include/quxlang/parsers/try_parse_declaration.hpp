//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_DECLARATION_HPP
#define TRY_PARSE_DECLARATION_HPP
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_asm_procedure.hpp>
#include <quxlang/parsers/try_parse_class.hpp>
#include <quxlang/parsers/try_parse_class_template.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>
#include <quxlang/parsers/try_parse_namespace.hpp>
#include <quxlang/parsers/try_parse_variable_declaration.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_class_declaration > try_parse_class(It& pos, It end);

    template < typename It >
    std::optional< ast2_declarable > try_parse_declarable(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        std::optional< ast2_declarable > output;
        output = try_parse_template(pos, end);
        if (output)
        {
            return output;
        }
        output = try_parse_function_declaration(pos, end);
        if (output)
        {
            return output;
        }
        output = try_parse_class(pos, end);
        if (output)
        {
            return output;
        }
        output = try_parse_variable_declaration(pos, end);

        if (output)
        {
            return output;
        }

        output = try_parse_asm_procedure_declaration(pos, end);
        if (output)
        {
            return output;
        }

        output = try_parse_namespace(pos, end);
        return output;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_DECLARATION_HPP
