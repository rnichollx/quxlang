// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Permissions is granted to use this software under the terms of the RPNX  Permissive License, Version 4, or, at your option, any later version as published by the author, Ryan P. Nicholl.

#ifndef QUXLANG_ORDERING_HEADER_GUARD
#define QUXLANG_ORDERING_HEADER_GUARD

#include <compare>

namespace quxlang
{
    template <typename T>
    std::strong_ordering strong_ordering_from_less(T const& lhs, T const& rhs)
    {
        if (lhs < rhs)
            return std::strong_ordering::less;
        if (rhs < lhs)
            return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

    template <typename T>
    bool equals_from_less(T const& lhs, T const& rhs)
    {
        return !(lhs < rhs) && !(rhs < lhs);
    }

    template <typename T>
    bool equals_from_spaceship(T const& lhs, T const& rhs)
    {
        return lhs <=> rhs == std::strong_ordering::equal;
    }
} // namespace quxlang

#endif // QUXLANG_ORDERING_HEADER_GUARD