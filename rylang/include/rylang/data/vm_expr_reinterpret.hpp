//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef RYLANG_VM_EXPR_REINTERPRET_HEADER_GUARD
#define RYLANG_VM_EXPR_REINTERPRET_HEADER_GUARD

#include "vm_expression.hpp"

namespace rylang
{
    struct vm_expr_reinterpret
    {
        vm_value expr;
        type_symbol type;
    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_REINTERPRET_HEADER_GUARD
