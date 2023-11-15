//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_LOAD_ADDRESS_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_LOAD_ADDRESS_HEADER

#include "rylang/data/qualified_symbol_reference.hpp"
#include <cstddef>

namespace rylang
{
    struct vm_expr_load_address
    {
        std::size_t index;
        qualified_symbol_reference type;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_LOAD_ADDRESS_HEADER
