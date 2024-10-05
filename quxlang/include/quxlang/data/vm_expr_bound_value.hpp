// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXPR_BOUND_VALUE_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_BOUND_VALUE_HEADER_GUARD

#include "vm_expression.hpp"
#include "vm_procedure_interface.hpp"

namespace quxlang
{
    struct vm_expr_bound_value
    {
        vm_value value;
        type_symbol function_ref;

        RPNX_MEMBER_METADATA(vm_expr_bound_value, value, function_ref);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_BOUND_VALUE_HEADER_GUARD
