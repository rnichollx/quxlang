//
// Created by Ryan Nicholl on 11/4/23.
//

#ifndef RPNX_RYANSCRIPT1031_ORDERING_HEADER
#define RPNX_RYANSCRIPT1031_ORDERING_HEADER

#include <compare>

namespace rylang
{
    template < typename T >
    std::strong_ordering strong_ordering_from_less(T const& lhs, T const& rhs)
    {
        if (lhs < rhs)
            return std::strong_ordering::less;
        if (rhs < lhs)
            return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

    template < typename T >
    bool equals_from_less(T const& lhs, T const& rhs)
    {
        return !(lhs < rhs) && !(rhs < lhs);
    }

    template <typename T>
    bool equals_from_spaceship(T const & lhs, T const & rhs)
    {
        return lhs <=> rhs == std::strong_ordering::equal;
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ORDERING_HEADER
