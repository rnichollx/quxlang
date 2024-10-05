// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXPR_ACCESS_FIELD_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_ACCESS_FIELD_HEADER_GUARD

#include "vm_expression.hpp"
#include "type_symbol.hpp"

namespace quxlang
{
    struct vm_expr_access_field
    {
        vm_value base;
        std::size_t offset;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_access_field, base, offset, type);
    };

    struct vm_exec_access_field
    {
        std::size_t base_index;
        std::size_t store_index;

        std::size_t offset;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_exec_access_field, base_index, store_index, offset, type);
    };


} // namespace quxlang

#endif // QUXLANG_VM_EXPR_ACCESS_FIELD_HEADER_GUARD