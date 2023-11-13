//
// Created by Ryan Nicholl on 11/12/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_BOUND_VALUE_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_BOUND_VALUE_HEADER

#include "vm_expression.hpp"
#include "vm_procedure_interface.hpp"

namespace rylang
{
    struct vm_expr_bound_value
    {
        vm_value value;
        qualified_symbol_reference function_ref;
        std::strong_ordering operator<=>(vm_expr_bound_value const &) const = default;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_BOUND_VALUE_HEADER
