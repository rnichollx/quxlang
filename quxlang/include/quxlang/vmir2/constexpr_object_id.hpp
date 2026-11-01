// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_CONSTEXPR_OBJECT_ID_HPP
#define QUXLANG_VMIR2_CONSTEXPR_OBJECT_ID_HPP

#include <compare>
#include <cstdint>
#include <limits>

namespace quxlang::vmir2
{
    /** Represents an unsigned 128-bit identity in the symbolic allocation address space. */
    struct constexpr_object_id
    {
        /// Most significant 64 bits of the identity.
        std::uint64_t high = 0;
        /// Least significant 64 bits of the identity.
        std::uint64_t low = 0;

        /** Compares the complete unsigned identities. */
        std::strong_ordering operator<=>(constexpr_object_id const&) const = default;

        /** Advances the identity, returning false without modification at the maximum value. */
        constexpr bool try_increment()
        {
            if (low != std::numeric_limits< std::uint64_t >::max())
            {
                ++low;
                return true;
            }
            if (high == std::numeric_limits< std::uint64_t >::max())
            {
                return false;
            }
            ++high;
            low = 0;
            return true;
        }
    };
}

#endif
