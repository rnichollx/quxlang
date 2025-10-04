// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_DECLARATION_HEADER_GUARD
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
#include <quxlang/parsers/parse_asm_procedure.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_class_declaration > try_parse_class(It& pos, It end);

    template < typename It >
    std::optional< ast2_top_declaration > try_parse_top_declaration(It& pos, It end);

    template < typename It >
    declaroid parse_declaroid(It& pos, It end);

    template < typename It >
    std::optional< declaroid > try_parse_declaroid(It& pos, It end);

    template < typename It >
    std::optional< subdeclaroid > try_parse_subdeclaroid(It& pos, It end);

    template < typename It >
    std::optional< ast2_namespace_declaration > try_parse_namespace(It& pos, It end);

    template < typename It >
    std::vector< subdeclaroid > parse_subdeclaroids(It& pos, It end)
    {
        std::vector< subdeclaroid > output;

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            std::optional< subdeclaroid > decl;
            decl = try_parse_subdeclaroid(pos, end);
            if (!decl)
            {
                break;
            }
            output.push_back(*decl);
        }

        return output;
    }



    template < typename It >
    std::optional< subdeclaroid > try_parse_subdeclaroid(It& pos, It end)
    {
        std::optional< subdeclaroid > output;

        auto name_opt = try_parse_name(pos, end);
        if (!name_opt)
        {
            return output;
        }
        auto [member, name] = *name_opt;

        skip_whitespace_and_comments(pos, end);
        std::string remaning(pos, end);

        auto ifl = try_parse_include_if(pos, end);


        auto decl = parse_declaroid(pos, end);

        if (member)
        {
            output = member_subdeclaroid {.decl = decl, .name = name, .include_if = ifl};
        }
        else
        {
            output = global_subdeclaroid {.decl = decl, .name = name, .include_if = ifl};
        }

        return output;
    }


    template < typename It >
    std::optional< declaroid > try_parse_declaroid(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        std::optional< declaroid > output;
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

        output = try_parse_static_test(pos, end);
        if (output)
        {
            return output;
        }

        output = try_parse_namespace(pos, end);
        return output;
    }

    template < typename It >
    std::optional< subdeclaroid > try_parse_subdeclaration(It& pos, It end)
    {


        auto decl = try_parse_named_declaration(pos, end);

        return decl;
    }



    template < typename It >
    declaroid parse_declaroid(It& pos, It end)
    {
        auto decl = try_parse_declaroid(pos, end);

        if (!decl.has_value())
        {
            throw std::logic_error("expected declaroid");
        }
        return std::move(decl.value());
    }

    template < typename It >
    std::optional< ast2_namespace_declaration > try_parse_namespace(It& pos, It end)
    {
        ast2_namespace_declaration out;

        if (!skip_keyword_if_is(pos, end, "NAMESPACE"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::logic_error("expected { after namespace");
        }

        auto decls = parse_subdeclaroids(pos, end);

        for (auto& decl : decls)
        {
            out.declarations.push_back(decl);
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::logic_error("expected } after namespace");
        }

        return out;
    }
} // namespace quxlang::parsers
#endif // RPNX_QUXLANG_DECLARATION_HEADER
