//
// Created by Ryan Nicholl on 10/23/23.
//

#ifndef RPNX_RYANSCRIPT1031_STRUCT_MATH_HEADER
#define RPNX_RYANSCRIPT1031_STRUCT_MATH_HEADER

#include <cstddef>

namespace rylang
{
    inline void advance_to_alignment(std::size_t & offset, std::size_t alignment)
    {
        if (alignment == 0 || alignment == 1) return;
        offset = (offset + alignment - 1) & ~(alignment - 1);
    }
}

#endif // RPNX_RYANSCRIPT1031_STRUCT_MATH_HEADER
