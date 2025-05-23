// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXPR_DEREFERENCE_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_DEREFERENCE_HEADER_GUARD

#include "vm_expression.hpp"
#include "vm_type.hpp"

namespace quxlang
{
    struct vm_expr_dereference
    {
        vm_value expr;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_dereference, expr, type);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_DEREFERENCE_HEADER_GUARD
