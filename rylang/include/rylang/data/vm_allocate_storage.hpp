//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_VM_ALLOCATE_STORAGE_HEADER_GUARD
#define RYLANG_VM_ALLOCATE_STORAGE_HEADER_GUARD

#include "rylang/data/qualified_symbol_reference.hpp"
#include <cstddef>

namespace rylang
{
    enum class storage_type { return_value, argument, local, temporary };
    struct vm_allocate_storage
    {
        std::size_t size = 0;
        std::size_t align = 0;
        type_symbol type;
        storage_type kind;

        bool valid() const
        {
            assert(kind == storage_type::return_value || kind == storage_type::argument || (align != 0 && size != 0));

            return kind == storage_type::return_value || kind == storage_type::argument || (align != 0 && size != 0);
        }
    };
} // namespace rylang

#endif // RYLANG_VM_ALLOCATE_STORAGE_HEADER_GUARD
