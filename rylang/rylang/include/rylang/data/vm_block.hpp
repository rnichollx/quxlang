//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_BLOCK_HEADER
#define RPNX_RYANSCRIPT1031_VM_BLOCK_HEADER

#include <cstddef>

#include "rylang/data/vm_executable_unit.hpp"

namespace rylang
{
    struct vm_block
    {
        std::vector<vm_executable_unit> code;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_BLOCK_HEADER
