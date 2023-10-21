//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_ABSOLUTE_LOOKUP_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_ABSOLUTE_LOOKUP_REFERENCE_HEADER

#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    struct absolute_lookup_reference
    {
        lookup_chain chain;
        bool operator <(absolute_lookup_reference const& other) const
        {
            return chain < other.chain;
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ABSOLUTE_LOOKUP_REFERENCE_HEADER
