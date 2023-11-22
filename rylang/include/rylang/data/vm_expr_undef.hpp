//
// Created by Ryan Nicholl on 11/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_UNDEF_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_UNDEF_HEADER

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    struct vm_expr_undef
    {
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_UNDEF_HEADER
