//
// Created by Ryan Nicholl on 10/21/23.
//

#ifndef QUXLANG_PROXIMATE_LOOKUP_REFERENCE_HEADER_GUARD
#define QUXLANG_PROXIMATE_LOOKUP_REFERENCE_HEADER_GUARD

#include "quxlang/data/lookup_chain.hpp"

namespace quxlang
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

} // namespace quxlang

#endif // QUXLANG_PROXIMATE_LOOKUP_REFERENCE_HEADER_GUARD
