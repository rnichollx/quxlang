//
// Created by Ryan Nicholl on 11/14/23.
//

#include "qualified_symbol_reference.hpp"

#ifndef QUXLANG_VM_EXPR_LOAD_LITERAL_HEADER_GUARD
#define QUXLANG_VM_EXPR_LOAD_LITERAL_HEADER_GUARD

#include <string>

namespace quxlang
{

    struct vm_expr_literal
    {
        std::string literal;

        std::strong_ordering operator<=>(const vm_expr_literal& other) const
        {
            return rpnx::compare(literal, other.literal);
        }
    };

    struct vm_expr_load_literal
    {
        std::string literal;
        type_symbol type;

        std::strong_ordering operator<=>(const vm_expr_load_literal& other) const
        {
            return rpnx::compare(literal, other.literal, type, other.type);
        }
    };


}

#endif // QUXLANG_VM_EXPR_LOAD_LITERAL_HEADER_GUARD