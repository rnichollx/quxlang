//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef PARSE_CLASS_BODY_HEADER_GUARD
#define PARSE_CLASS_BODY_HEADER_GUARD

#include <optional>
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/parsers/parse_named_declarations.hpp>
#include <quxlang/parsers/try_parse_class_function_declaration.hpp>
#include <quxlang/parsers/try_parse_class_variable_declaration.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::vector< ast2_named_declaration > parse_named_declarations(It& pos, It end);

    template < typename It >
    std::vector< ast2_top_declaration > parse_top_declarations(It& pos, It end);

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
        auto declarations = parse_top_declarations(pos, end);

        for (ast2_top_declaration const & decl: declarations)
        {
            result.declarations.push_back(decl);
        }

        skip_whitespace_and_comments(pos, end);

        std::string rem (pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::runtime_error("Expected '}'");
        }

        return result;
    }
} // namespace quxlang::parsers

#endif // PARSE_CLASS_BODY_HEADER_GUARD
