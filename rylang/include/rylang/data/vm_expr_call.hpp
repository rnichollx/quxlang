//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_CALL_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_CALL_HEADER

#include "vm_expression.hpp"
#include "vm_procedure_interface.hpp"

namespace rylang
{
    struct vm_expr_call
    {
        std::string mangled_procedure_name;
        qualified_symbol_reference functanoid;
        // TODO: Is this interface needed?
        vm_procedure_interface interface;
        std::vector< vm_value > arguments;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_CALL_HEADER
