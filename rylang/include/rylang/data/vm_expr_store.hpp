//
// Created by Ryan Nicholl on 11/7/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_STORE_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_STORE_HEADER

#include "vm_expression.hpp"
namespace rylang
{
    struct vm_expr_store
    {
        vm_value what;
        vm_value where;
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_STORE_HEADER
