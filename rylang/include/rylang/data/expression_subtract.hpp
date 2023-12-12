//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RYLANG_EXPRESSION_SUBTRACT_HEADER_GUARD
#define RYLANG_EXPRESSION_SUBTRACT_HEADER_GUARD

#include "expression.hpp"
#include <compare>

namespace rylang
{
    struct expression_subtract
    {
        static constexpr const char * name = "subtract";
        static constexpr const char * symbol = "-";
        static constexpr const int priority = 4;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_subtract& other) const  = default;
    };
} // namespace rylang

#endif // RYLANG_EXPRESSION_SUBTRACT_HEADER_GUARD
