//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_ADD_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_ADD_HEADER

#include "rylang/cow.hpp"
#include "rylang/data/expression.hpp"

namespace rylang
{
    struct expression_add
    {
        static constexpr const char* const symbol = "+";
        static constexpr const char* const name = "add";
        static constexpr const int priority = 4;

        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_add& other) const = default;
    };

    struct expression_addp
    {
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_addp& other) const = default;
    };

    struct expression_addw
    {
        std::strong_ordering operator<=>(const expression_addw& other) const = default;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_ADD_HEADER
