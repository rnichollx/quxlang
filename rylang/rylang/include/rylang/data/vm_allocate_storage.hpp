//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_ALLOCATE_STORAGE_HEADER
#define RPNX_RYANSCRIPT1031_VM_ALLOCATE_STORAGE_HEADER

#include <cstddef>

namespace rylang
{
    struct vm_allocate_storage
    {
        std::size_t size = 0;
        std::size_t align = 0;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_ALLOCATE_STORAGE_HEADER
