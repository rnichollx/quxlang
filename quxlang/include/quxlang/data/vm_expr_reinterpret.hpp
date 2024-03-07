//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef QUXLANG_VM_EXPR_REINTERPRET_HEADER_GUARD
#define QUXLANG_VM_EXPR_REINTERPRET_HEADER_GUARD

#include "vm_expression.hpp"

namespace quxlang
{
    struct vm_expr_reinterpret
    {
        vm_value expr;
        type_symbol type;

        std::strong_ordering operator<=>(const vm_expr_reinterpret&other) const
        {
            return rpnx::compare(expr, other.expr, type, other.type);
        }
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_REINTERPRET_HEADER_GUARD
