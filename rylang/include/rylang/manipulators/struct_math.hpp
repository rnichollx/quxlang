//
// Created by Ryan Nicholl on 10/23/23.
//

#ifndef RYLANG_STRUCT_MATH_HEADER_GUARD
#define RYLANG_STRUCT_MATH_HEADER_GUARD

#include <cstddef>

namespace rylang
{
    inline void advance_to_alignment(std::size_t & offset, std::size_t alignment)
    {
        if (alignment == 0 || alignment == 1) return;
        offset = (offset + alignment - 1) & ~(alignment - 1);
    }
}

#endif // RYLANG_STRUCT_MATH_HEADER_GUARD
