// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXPR_STORE_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_STORE_HEADER_GUARD

#include "vm_expression.hpp"
namespace quxlang
{
    struct vm_expr_store
    {
        vm_value what;
        vm_value where;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_store, what, where, type);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_STORE_HEADER_GUARD
