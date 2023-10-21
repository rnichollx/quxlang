//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_POINTER_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_POINTER_REFERENCE_HEADER

#include "type_reference.hpp"

namespace rylang
{
    struct pointer_reference
    {
        type_reference to;

        inline bool operator <(pointer_reference const& other) const
        {
            return to < other.to;
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_POINTER_REFERENCE_HEADER
