//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RYLANG_ABSOLUTE_LOOKUP_REFERENCE_HEADER_GUARD
#define RYLANG_ABSOLUTE_LOOKUP_REFERENCE_HEADER_GUARD

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

#endif // RYLANG_ABSOLUTE_LOOKUP_REFERENCE_HEADER_GUARD
