//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_NAMESPACE_HPP
#define TRY_PARSE_NAMESPACE_HPP
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_named_declarations.hpp>

namespace quxlang::parsers
{

    template < typename It >
    std::vector< ast2_top_declaration > parse_top_declarations(It& pos, It end);

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
            throw std::runtime_error("expected { after namespace");
        }

        auto decls = parse_top_declarations(pos, end);

        for (auto& decl : decls)
        {
            out.declarations.push_back(decl);
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::runtime_error("expected } after namespace");
        }

        return out;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_NAMESPACE_HPP
