//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef PARSE_DECLARATION_HPP
#define PARSE_DECLARATION_HPP
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/try_parse_declaration.hpp>

namespace quxlang::parsers
{
    template < typename It >
    ast2_declarable parse_declaration(It& pos, It end)
    {
        return try_parse_declarable(pos, end).value();
    }
} // namespace quxlang::parsers
#endif // PARSE_DECLARATION_HPP
