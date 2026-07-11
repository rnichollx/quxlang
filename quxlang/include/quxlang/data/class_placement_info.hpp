// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_PLACEMENT_INFO_HEADER_GUARD
#define QUXLANG_DATA_CLASS_PLACEMENT_INFO_HEADER_GUARD

#include <cstddef>

#include <rpnx/macros.hpp>

namespace quxlang
{
    /** Describes the size and alignment of a class object. */
    struct class_placement_info
    {
        std::uint64_t size;
        std::uint64_t alignment;

        RPNX_MEMBER_METADATA(class_placement_info, size, alignment);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_CLASS_PLACEMENT_INFO_HEADER_GUARD
