//
// Created by Ryan Nicholl on 11/20/23.
//

#ifndef RYLANG_VM_EXPR_UNDEF_HEADER_GUARD
#define RYLANG_VM_EXPR_UNDEF_HEADER_GUARD

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    struct vm_expr_undef
    {
        type_symbol type;
    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_UNDEF_HEADER_GUARD
