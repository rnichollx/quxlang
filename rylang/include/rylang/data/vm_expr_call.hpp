//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_VM_EXPR_CALL_HEADER_GUARD
#define RYLANG_VM_EXPR_CALL_HEADER_GUARD

#include "vm_expression.hpp"
#include "vm_procedure_interface.hpp"

namespace rylang
{
    struct vm_expr_call
    {
        std::string mangled_procedure_name;
        type_symbol functanoid;
        // TODO: Is this interface needed?
        vm_procedure_interface interface;
        std::vector< vm_value > arguments;
    };
} // namespace rylang

#endif // RYLANG_VM_EXPR_CALL_HEADER_GUARD
