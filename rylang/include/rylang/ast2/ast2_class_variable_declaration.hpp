//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef RYLANG_AST2_CLASS_VARIABLE_DECLARATION_HEADER_GUARD
#define RYLANG_AST2_CLASS_VARIABLE_DECLARATION_HEADER_GUARD

#include "rylang/data/expression.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

#include <compare>

namespace rylang
{


    struct ast2_class_variable_declaration
    {
        std::string name;
        bool is_member = false;
        type_symbol type;
        std::optional< std::size_t > offset;
        std::optional< expression > enable_if;
        std::strong_ordering operator<=>(const ast2_class_variable_declaration& other) const = default;
    };
}

#endif //AST2_VARIABLE_HEADER_GUARD
