//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef RPNX_QUXLANG_DECLARATION_HEADER
#define RPNX_QUXLANG_DECLARATION_HEADER
#include <optional>
#include <quxlang/ast2/ast2_entity.hpp>

#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/include_if.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/try_parse_class.hpp>
#include <quxlang/parsers/try_parse_class_template.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>
#include <quxlang/parsers/try_parse_name.hpp>
#include <quxlang/parsers/try_parse_variable_declaration.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_class_declaration > try_parse_class(It& pos, It end);

    template < typename It >
    std::optional< ast2_top_declaration > try_parse_top_declaration(It& pos, It end);

    template < typename It >
    ast2_declarable parse_declarable(It& pos, It end);

    template < typename It >
    std::optional< ast2_declarable > try_parse_declarable(It& pos, It end);

    template < typename It >
    std::vector< ast2_top_declaration > parse_top_declarations(It& pos, It end)
    {
        std::vector< ast2_top_declaration > output;

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            std::optional< ast2_top_declaration > decl;
            decl = try_parse_top_declaration(pos, end);
            if (!decl)
            {
                break;
            }
            output.push_back(*decl);
        }

        return output;
    }

    template < typename It >
    std::optional< ast2_named_declaration > try_parse_named_declaration(It& pos, It end)
    {
        std::optional< ast2_named_declaration > output;

        auto name_opt = try_parse_name(pos, end);
        if (!name_opt)
        {
            return output;
        }
        auto [member, name] = *name_opt;

        skip_whitespace_and_comments(pos, end);
        std::string remaning(pos, end);

        auto decl = parse_declarable(pos, end);

        if (member)
        {
            ast2_named_member m{};
            m.name = name;
            m.declaration = decl;
            output = m;
        }
        else
        {
            ast2_named_global g{};
            g.name = name;
            g.declaration = decl;
            output = g;
        }
        return output;
    }

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

    template < typename It >
    std::optional< ast2_top_declaration > try_parse_top_declaration(It& pos, It end)
    {
        std::optional< ast2_include_if > include_if = try_parse_include_if(pos, end);
        if (include_if)

        {
            return include_if;
        }

        auto decl = try_parse_named_declaration(pos, end);

        return decl;
    }

    template < typename It >
    ast2_declarable parse_declarable(It& pos, It end)
    {
        return try_parse_declarable(pos, end).value();
    }
} // namespace quxlang::parsers
#endif // RPNX_QUXLANG_DECLARATION_HEADER
