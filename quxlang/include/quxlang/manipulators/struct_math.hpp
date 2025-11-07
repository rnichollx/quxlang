// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_STRUCT_MATH_HEADER_GUARD
#define QUXLANG_MANIPULATORS_STRUCT_MATH_HEADER_GUARD

#include <cstddef>

namespace quxlang
{
    inline void advance_to_alignment(std::uint64_t & offset, std::uint64_t alignment)
    {
        if (alignment == 0 || alignment == 1) return;
        offset = (offset + alignment - 1) & ~(alignment - 1);
    }
}

#endif // QUXLANG_STRUCT_MATH_HEADER_GUARD
