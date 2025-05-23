// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_ALLOCATE_STORAGE_HEADER_GUARD
#define QUXLANG_DATA_VM_ALLOCATE_STORAGE_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <compare>
#include <cstddef>

#include <rpnx/serializer.hpp>
#include <rpnx/metadata.hpp>

RPNX_ENUM(quxlang, storage_type, std::int16_t, return_value, argument, local, temporary);

namespace quxlang
{
    //enum class storage_type { return_value, argument, local, temporary };

    struct vm_allocate_storage
    {
        std::size_t size = 0;
        std::size_t align = 0;
        type_symbol type;
        storage_type kind;

        RPNX_MEMBER_METADATA(vm_allocate_storage, size, align, type, kind);

        bool valid() const { return true; }
    };
} // namespace quxlang

#endif // QUXLANG_VM_ALLOCATE_STORAGE_HEADER_GUARD
