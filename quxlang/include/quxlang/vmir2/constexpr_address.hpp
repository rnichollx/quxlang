// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_CONSTEXPR_ADDRESS_HEADER_GUARD
#define QUXLANG_VMIR2_CONSTEXPR_ADDRESS_HEADER_GUARD

#include <quxlang/vmir2/constexpr_object_id.hpp>

#include <compare>
#include <cstdint>
#include <limits>

namespace quxlang::vmir2
{
    /** Identifies a byte position independently of the object occupying its storage. */
    struct constexpr_address
    {
        constexpr_object_id allocation;
        std::uint64_t offset = 0;

        /** Orders allocations and then physical byte positions within each allocation. */
        std::strong_ordering operator<=>(constexpr_address const&) const = default;

        /** Advances within the allocation's byte-offset representation without wrapping. */
        constexpr bool try_advance(std::uint64_t amount)
        {
            if (amount > std::numeric_limits< std::uint64_t >::max() - offset) return false;
            offset += amount;
            return true;
        }
    };
}

#endif // QUXLANG_VMIR2_CONSTEXPR_ADDRESS_HEADER_GUARD
