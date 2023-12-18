//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef AST2_NAMED_DECLARATION_HPP
#define AST2_NAMED_DECLARATION_HPP
#include <rylang/ast2/ast2_entity.hpp>

namespace rylang
{
    struct ast2_named_global
    {
        std::string name;
        ast2_declarable declaration;
        std::strong_ordering operator<=>(const ast2_named_global& other) const = default;
    };

    struct ast2_named_member
    {
        std::string name;
        ast2_declarable declaration;
        std::strong_ordering operator<=>(const ast2_named_member& other) const = default;
    };

    using ast2_named_declaration = boost::variant< ast2_named_global, ast2_named_member >;
}; // namespace rylang

#endif // AST2_NAMED_DECLARATION_HPP
