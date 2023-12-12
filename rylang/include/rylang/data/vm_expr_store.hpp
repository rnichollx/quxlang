//
// Created by Ryan Nicholl on 11/7/23.
//

#ifndef RYLANG_VM_EXPR_STORE_HEADER_GUARD
#define RYLANG_VM_EXPR_STORE_HEADER_GUARD

#include "vm_expression.hpp"
namespace rylang
{
    struct vm_expr_store
    {
        vm_value what;
        vm_value where;
        type_symbol type;
    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_STORE_HEADER_GUARD
