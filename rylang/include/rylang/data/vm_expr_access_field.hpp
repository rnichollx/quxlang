//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef RYLANG_VM_EXPR_ACCESS_FIELD_HEADER_GUARD
#define RYLANG_VM_EXPR_ACCESS_FIELD_HEADER_GUARD

#include "vm_expression.hpp"

namespace rylang
{
    struct vm_expr_access_field
    {
        vm_value base;
        std::size_t offset;
        type_symbol type;
    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_ACCESS_FIELD_HEADER_GUARD
