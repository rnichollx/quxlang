//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_VM_EXPR_LOAD_ADDRESS_HEADER_GUARD
#define RYLANG_VM_EXPR_LOAD_ADDRESS_HEADER_GUARD

#include "rylang/data/qualified_symbol_reference.hpp"
#include <cstddef>

namespace rylang
{
    struct vm_expr_load_address
    {
        std::size_t index;
        type_symbol type;
    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_LOAD_ADDRESS_HEADER_GUARD
