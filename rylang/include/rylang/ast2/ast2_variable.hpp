//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef AST2_VARIABLE_HPP
#define AST2_VARIABLE_HPP

#include "rylang/data/expression.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

#include <compare>

namespace rylang
{
    struct ast2_variable_declaration
    {
        std::string name;
        qualified_symbol_reference type;
        std::optional< std::size_t > offset;
        std::optional< expression > enable_if;


        std::strong_ordering operator<=>(const ast2_variable_declaration& other) const = default;

    };
}

#endif //AST2_VARIABLE_HPP
