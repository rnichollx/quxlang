//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_EXPR_LOAD_ADDRESS_HEADER_GUARD
#define QUXLANG_VM_EXPR_LOAD_ADDRESS_HEADER_GUARD

#include "quxlang/data/qualified_symbol_reference.hpp"
#include <cstddef>

namespace quxlang
{
    struct vm_expr_load_address
    {
        std::size_t index;
        type_symbol type;

        auto operator<=>(const vm_expr_load_address& other) const
        {
            return rpnx::compare(index, other.index, type, other.type);
        }
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_LOAD_ADDRESS_HEADER_GUARD
