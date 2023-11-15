//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_ACCESS_FIELD_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_ACCESS_FIELD_HEADER

#include "vm_expression.hpp"

namespace rylang
{
    struct vm_expr_access_field
    {
        vm_value base;
        std::size_t offset;
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_ACCESS_FIELD_HEADER
