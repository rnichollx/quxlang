//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_NAMESPACE_HPP
#define TRY_PARSE_NAMESPACE_HPP
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_named_declarations.hpp>
#include <quxlang/parsers/keyword.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::vector< ast2_named_declaration > parse_named_declarations(It& pos, It end);

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

        auto decls = parse_named_declarations(pos, end);

        for (auto& decl : decls)
        {
            if (typeis< ast2_named_global >(decl))
            {
                auto ng = as< ast2_named_global >(decl);
                out.globals.push_back({ng.name, ng.declaration});
            }
            else
            {
                throw std::runtime_error("unexpected element inside namespace");
            }
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
