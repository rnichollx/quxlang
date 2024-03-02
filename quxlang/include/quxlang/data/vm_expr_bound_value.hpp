//
// Created by Ryan Nicholl on 11/12/23.
//

#ifndef QUXLANG_VM_EXPR_BOUND_VALUE_HEADER_GUARD
#define QUXLANG_VM_EXPR_BOUND_VALUE_HEADER_GUARD

#include "vm_expression.hpp"
#include "vm_procedure_interface.hpp"

namespace quxlang
{
    struct vm_expr_bound_value
    {
        vm_value value;
        type_symbol function_ref;
        std::strong_ordering operator<=>(vm_expr_bound_value const &) const = default;
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_BOUND_VALUE_HEADER_GUARD
