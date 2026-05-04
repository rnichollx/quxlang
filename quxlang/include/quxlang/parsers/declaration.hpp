// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_DECLARATION_HEADER_GUARD
#include <optional>
#include <utility>
#include <quxlang/ast2/ast2_entity.hpp>

#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/doc.hpp>
#include <quxlang/parsers/include_if.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/try_parse_class.hpp>
#include <quxlang/parsers/try_parse_interface.hpp>
#include <quxlang/parsers/try_parse_template_declaration.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>
#include <quxlang/parsers/try_parse_name.hpp>
#include <quxlang/parsers/try_parse_variable_declaration.hpp>
#include <quxlang/parsers/parse_asm_procedure.hpp>
#include <quxlang/parsers/option.hpp>

namespace quxlang::parsers
{
    std::optional< quxlang::ast2_template_declaration > try_parse_template(parsing_context& ctx);
    std::optional< ast2_class_declaration > try_parse_class(parsing_context& ctx);
    std::optional< ast2_interface_declaration > try_parse_interface(parsing_context& ctx);
    std::optional< ast2_implementation_declaration > try_parse_implementation(parsing_context& ctx);
    std::optional< ast2_function_declaration > try_parse_function_declaration(parsing_context& ctx);
    std::optional< ast2_static_test > try_parse_static_test(parsing_context& ctx);
    std::optional< ast2_variable_declaration > try_parse_variable_declaration(parsing_context& ctx);
    std::optional< ast2_option > try_parse_option(parsing_context& ctx);
    std::optional< declaroid > try_parse_declaroid(parsing_context& ctx);
    declaroid parse_declaroid(parsing_context& ctx);
    std::optional< subdeclaroid > try_parse_subdeclaroid(parsing_context& ctx);
    std::optional< ast2_namespace_declaration > try_parse_namespace(parsing_context& ctx);

    inline std::vector< subdeclaroid > parse_subdeclaroids(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        std::vector< subdeclaroid > output;

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            std::optional< subdeclaroid > decl;
            decl = try_parse_subdeclaroid(ctx);
            if (!decl)
            {
                break;
            }
            output.push_back(std::move(*decl));
        }

        return output;
    }

    inline std::optional< subdeclaroid > try_parse_subdeclaroid(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        std::optional< subdeclaroid > output;

        auto name_opt = try_parse_name(pos, end);
        if (!name_opt)
        {
            return output;
        }
        auto [member, name] = std::move(*name_opt);

        skip_whitespace_and_comments(pos, end);

        auto ifl = try_parse_include_if(ctx);
        auto doc = try_parse_single_doc(ctx);

        auto decl = parse_declaroid(ctx);

        if (member)
        {
            output = member_subdeclaroid {
                .decl = std::move(decl),
                .name = std::move(name),
                .include_if = std::move(ifl),
                .doc = std::move(doc),
                .location = ctx.get_location_optional(begin, pos)};
        }
        else
        {
            output = global_subdeclaroid {
                .decl = std::move(decl),
                .name = std::move(name),
                .include_if = std::move(ifl),
                .doc = std::move(doc),
                .location = ctx.get_location_optional(begin, pos)};
        }

        return output;
    }

    inline std::optional< declaroid > try_parse_declaroid(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        skip_whitespace_and_comments(pos, end);
        std::optional< declaroid > output;
        output = try_parse_template(ctx);
        if (output)
        {
            return std::move(output);
        }
        output = try_parse_function_declaration(ctx);
        if (output)
        {
            return std::move(output);
        }
        output = try_parse_class(ctx);
        if (output)
        {
            return std::move(output);
        }
        output = try_parse_interface(ctx);
        if (output)
        {
            return std::move(output);
        }
        output = try_parse_implementation(ctx);
        if (output)
        {
            return std::move(output);
        }
        output = try_parse_option(ctx);
        if (output)
        {
            return std::move(output);
        }
        output = try_parse_variable_declaration(ctx);

        if (output)
        {
            return std::move(output);
        }

        output = try_parse_asm_procedure_declaration(ctx);
        if (output)
        {
            return std::move(output);
        }

        output = try_parse_static_test(ctx);
        if (output)
        {
            return std::move(output);
        }

        output = try_parse_namespace(ctx);
        return output;
    }

    inline declaroid parse_declaroid(parsing_context& ctx)
    {
        auto decl = try_parse_declaroid(ctx);

        if (!decl.has_value())
        {
            throw std::logic_error("expected declaroid");
        }
        return std::move(*decl);
    }

    inline std::optional< ast2_namespace_declaration > try_parse_namespace(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
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

        out.declarations = parse_subdeclaroids(ctx);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::logic_error("expected } after namespace");
        }

        out.location = ctx.get_location_optional(begin, pos);
        return out;
    }

} // namespace quxlang::parsers
#endif // RPNX_QUXLANG_DECLARATION_HEADER
