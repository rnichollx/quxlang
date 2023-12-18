//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef PARSE_CLASS_BODY_HEADER_GUARD
#define PARSE_CLASS_BODY_HEADER_GUARD

#include <optional>
#include <rylang/ast2/ast2_type_map.hpp>
#include <rylang/parsers/parse_named_declarations.hpp>
#include <rylang/parsers/try_parse_class_function_declaration.hpp>
#include <rylang/parsers/try_parse_class_variable_declaration.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::vector< ast2_named_declaration > parse_named_declarations(It& pos, It end);

    template < typename It >
    ast2_class_declaration parse_class_body(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        ast2_class_declaration result;
        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::runtime_error("Expected '{'");
        }

    member:

        skip_whitespace_and_comments(pos, end);
        auto declarations = parse_named_declarations(pos, end);

        for (auto& decl: declarations)
        {
            if (typeis<ast2_named_member>(decl))
            {
                auto member = as<ast2_named_member>(decl);
                result.members.push_back({member.name, member.declaration});
            }
            else if (typeis<ast2_named_global>(decl))
            {
                auto global = as<ast2_named_global>(decl);
                result.globals.push_back({global.name, global.declaration});
            }
            else
            {
                rpnx::unimplemented();
            }
        }

        std::string rem (pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::runtime_error("Expected '}'");
        }

        return result;
    }
} // namespace rylang::parsers

#endif // PARSE_CLASS_BODY_HEADER_GUARD
