//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_DATA_VM_PROCEDURE_HEADER_GUARD
#define QUXLANG_DATA_VM_PROCEDURE_HEADER_GUARD

#include "vm_block.hpp"
#include "vm_procedure_interface.hpp"
#include <cstddef>

#include <rpnx/compare.hpp>
#include <rpnx/serializer.hpp>

namespace quxlang
{
    struct vm_procedure
    {
        std::string mangled_name;
        vm_block body;
        vm_procedure_interface interface;
        std::vector< vm_allocate_storage > storage;
        std::set< type_symbol > invoked_functanoids;
        std::set< type_symbol > invoked_asm_procedures;
        std::set< std::string > invoked_externs;

        RPNX_MEMBER_METADATA(vm_procedure, mangled_name, body, interface, storage, invoked_functanoids, invoked_asm_procedures, invoked_externs);
    };
} // namespace quxlang

#endif // QUXLANG_VM_PROCEDURE_HEADER_GUARD
