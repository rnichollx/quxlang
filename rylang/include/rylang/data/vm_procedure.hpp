//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_PROCEDURE_HEADER
#define RPNX_RYANSCRIPT1031_VM_PROCEDURE_HEADER

#include "vm_block.hpp"
#include <cstddef>
#include "vm_procedure_interface.hpp"

namespace rylang
{
    struct vm_procedure
    {
        vm_block body;
        vm_procedure_interface interface;
        std::vector<vm_allocate_storage> storage;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_HEADER
