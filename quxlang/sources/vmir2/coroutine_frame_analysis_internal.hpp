// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_COROUTINE_FRAME_ANALYSIS_INTERNAL_HEADER_GUARD
#define QUXLANG_VMIR2_COROUTINE_FRAME_ANALYSIS_INTERNAL_HEADER_GUARD

#include <cstddef>
#include <cstdint>

namespace quxlang::vmir2
{
    /** Stores one interval allocator cell whose occupants have disjoint inclusive lifetimes. */
    struct coroutine_frame_cell
    {
        std::uint64_t size = 0;
        std::uint64_t alignment = 1;
        std::size_t size_class = 0;
        std::uint64_t offset = 0;
    };
}

#endif // QUXLANG_VMIR2_COROUTINE_FRAME_ANALYSIS_INTERNAL_HEADER_GUARD
