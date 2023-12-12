//
// Created by Ryan Nicholl on 10/21/23.
//

#ifndef RYLANG_PROXIMATE_LOOKUP_REFERENCE_HEADER_GUARD
#define RYLANG_PROXIMATE_LOOKUP_REFERENCE_HEADER_GUARD

#include "rylang/data/lookup_chain.hpp"

namespace rylang
{

    struct [[deprecated("qualified")]] proximate_lookup_reference
    {
        lookup_chain chain;

        std::strong_ordering operator<=>(const proximate_lookup_reference& other) const
        {
            return strong_ordering_from_less(chain, other.chain);
        }

        bool operator==(const proximate_lookup_reference& other) const
        {
            return *this <=> other == std::strong_ordering::equal;
        }

        bool operator!=(const proximate_lookup_reference& other) const
        {
            return *this <=> other != std::strong_ordering::equal;
        }


    };

} // namespace rylang

#endif // RYLANG_PROXIMATE_LOOKUP_REFERENCE_HEADER_GUARD
