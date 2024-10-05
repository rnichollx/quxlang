//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef QUXLANG_PARSERS_PARSE_CLASS_BODY_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_CLASS_BODY_HEADER_GUARD

#include <optional>
#include <quxlang/ast2/ast2_type_map.hpp>
#include <quxlang/parsers/try_parse_class_function_declaration.hpp>
#include <quxlang/parsers/try_parse_class_variable_declaration.hpp>
#include <quxlang/parsers/declaration.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::vector< ast2_named_declaration > parse_named_declarations(It& pos, It end);

    template <typename It>
    std::vector< subdeclaroid > parse_subdeclaroids(It& pos, It end);

    template < typename It >
    ast2_class_declaration parse_class_body(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        ast2_class_declaration result;
        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::logic_error("Expected '{'");
        }

    member:

        skip_whitespace_and_comments(pos, end);
        auto subdecls = parse_subdeclaroids(pos, end);

        for ( subdeclaroid const & decl: subdecls)
        {
            result.declarations.push_back(decl);
        }

        skip_whitespace_and_comments(pos, end);

        std::string rem (pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::logic_error("Expected '}'");
        }

        return result;
    }
} // namespace quxlang::parsers

#endif // PARSE_CLASS_BODY_HEADER_GUARD
