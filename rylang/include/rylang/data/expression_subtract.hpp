//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_SUBTRACT_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_SUBTRACT_HEADER

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

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_SUBTRACT_HEADER
