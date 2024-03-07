//
// Created by Ryan Nicholl on 11/7/23.
//

#ifndef QUXLANG_VM_EXPR_STORE_HEADER_GUARD
#define QUXLANG_VM_EXPR_STORE_HEADER_GUARD

#include "vm_expression.hpp"
namespace quxlang
{
    struct vm_expr_store
    {
        vm_value what;
        vm_value where;
        type_symbol type;

        std::strong_ordering operator<=>(const vm_expr_store& other) const
        {
            return rpnx::compare(what, other.what, where, other.where, type, other.type);
        }
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_STORE_HEADER_GUARD
