//
// Created by Ryan Nicholl on 11/4/23.
// Copyright (c) 2023 Ryan P. Nicholl <rnicholl@protonmail.com>
// Permissions is granted to use this software under the terms of the RPNX  Permissive License, Version 4, or, at your option, any later version as published by the author, Ryan P. Nicholl.

#ifndef RYLANG_ORDERING_HEADER_GUARD
#define RYLANG_ORDERING_HEADER_GUARD

#include <compare>

namespace rylang
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
} // namespace rylang

#endif // RYLANG_ORDERING_HEADER_GUARD