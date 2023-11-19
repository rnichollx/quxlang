//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_REINTERPRET_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_REINTERPRET_HEADER

#include "vm_expression.hpp"

namespace rylang
{
    struct vm_expr_reinterpret
    {
        vm_value expr;
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_REINTERPRET_HEADER
