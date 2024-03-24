//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_EXPR_CALL_HEADER_GUARD
#define QUXLANG_VM_EXPR_CALL_HEADER_GUARD

#include "vm_expression.hpp"
#include "vm_procedure_interface.hpp"

namespace quxlang
{
    struct vm_invoke
    {
        std::string mangled_procedure_name;
        type_symbol functanoid;

        // TODO: Is this interface needed?
        vm_procedure_interface interface;
        std::vector< vm_value > arguments;


        RPNX_MEMBER_METADATA(vm_invoke, mangled_procedure_name, functanoid, interface, arguments);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_CALL_HEADER_GUARD
