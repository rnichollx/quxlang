//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef PARSE_DECLARATION_HPP
#define PARSE_DECLARATION_HPP
#include <rylang/ast2/ast2_entity.hpp>
#include <rylang/parsers/try_parse_declaration.hpp>

namespace rylang::parsers
{
    template < typename It >
    ast2_declarable parse_declaration(It& pos, It end)
    {
        return try_parse_declarable(pos, end).value();
    }
} // namespace rylang::parsers
#endif // PARSE_DECLARATION_HPP
