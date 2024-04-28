//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD
#define QUXLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD

#include "vm_type.hpp"
#include <optional>
#include <vector>

#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    struct vm_procedure_interface
    {
        // TODO: replace with call_type
        std::optional< type_symbol > return_type;
        call_type argument_types;

        RPNX_MEMBER_METADATA(vm_procedure_interface, return_type, argument_types);
    };
} // namespace quxlang

#endif // QUXLANG_VM_PROCEDURE_INTERFACE_HEADER_GUARD
