//
// Created by Ryan Nicholl on 10/31/23.
//

#ifndef RYLANG_VM_EXPR_DEREFERENCE_HEADER_GUARD
#define RYLANG_VM_EXPR_DEREFERENCE_HEADER_GUARD

#include "vm_expression.hpp"
#include "vm_type.hpp"

namespace rylang
{
    struct vm_expr_dereference
    {
        vm_value expr;
        type_symbol type;
        //vm_type type;

    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_DEREFERENCE_HEADER_GUARD
