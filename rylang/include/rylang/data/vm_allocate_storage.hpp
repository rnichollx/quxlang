//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_ALLOCATE_STORAGE_HEADER
#define RPNX_RYANSCRIPT1031_VM_ALLOCATE_STORAGE_HEADER

#include "rylang/data/qualified_symbol_reference.hpp"
#include <cstddef>

namespace rylang
{
    enum class storage_type { return_value, argument, local, temporary };
    struct vm_allocate_storage
    {
        std::size_t size = 0;
        std::size_t align = 0;
        qualified_symbol_reference type;
        storage_type kind;

        bool valid() const
        {
            assert(kind == storage_type::return_value || kind == storage_type::argument || (align != 0 && size != 0));

            return kind == storage_type::return_value || kind == storage_type::argument || (align != 0 && size != 0);
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_ALLOCATE_STORAGE_HEADER
