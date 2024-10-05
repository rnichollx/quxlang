//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_DATA_VM_EXPR_LOAD_REFERENCE_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_LOAD_REFERENCE_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <cstddef>

namespace quxlang
{
    struct vm_expr_load_reference
    {
        std::size_t index;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_load_reference, index, type);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_LOAD_ADDRESS_HEADER_GUARD
