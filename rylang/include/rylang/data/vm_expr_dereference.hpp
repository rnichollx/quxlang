//
// Created by Ryan Nicholl on 10/31/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_DEREFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_DEREFERENCE_HEADER

#include "vm_expression.hpp"
#include "vm_type.hpp"

namespace rylang
{
    struct vm_expr_dereference
    {
        vm_value expr;
        qualified_symbol_reference type;
        //vm_type type;

    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_DEREFERENCE_HEADER
