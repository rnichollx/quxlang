//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_EXPR_CALL_HEADER_GUARD
#define QUXLANG_VM_EXPR_CALL_HEADER_GUARD

#include "vm_expression.hpp"
#include "vm_procedure_interface.hpp"

namespace quxlang
{
    // deprecated
    struct vm_invoke
    {
        std::string mangled_procedure_name;
        type_symbol functanoid;

        vm_procedure_interface interface;
        vm_callargs arguments;


        RPNX_MEMBER_METADATA(vm_invoke, mangled_procedure_name, functanoid, interface, arguments);
    };
} // namespace quxlang

#endif // QUXLANG_VM_EXPR_CALL_HEADER_GUARD
