//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef QUXLANG_VM_EXPR_ACCESS_FIELD_HEADER_GUARD
#define QUXLANG_VM_EXPR_ACCESS_FIELD_HEADER_GUARD

#include "vm_expression.hpp"

namespace quxlang
{
    struct vm_expr_access_field
    {
        vm_value base;
        std::size_t offset;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_access_field, base, offset, type);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_ACCESS_FIELD_HEADER_GUARD