//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_ABSOLUTE_LOOKUP_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_ABSOLUTE_LOOKUP_REFERENCE_HEADER

#include "rylang/data/lookup_chain.hpp"
#include "rylang/ordering.hpp"

namespace rylang
{
    struct [[deprecated("qualified")]] absolute_lookup_reference
    {
        lookup_chain chain;

        std::strong_ordering operator<=>(const absolute_lookup_reference& other) const
        {
            return strong_ordering_from_less(chain, other.chain);
        }

        bool operator==(const absolute_lookup_reference& other) const
        {
            return *this <=> other == std::strong_ordering::equal;
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ABSOLUTE_LOOKUP_REFERENCE_HEADER
