//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RYLANG_EXPRESSION_MULTIPLY_HEADER_GUARD
#define RYLANG_EXPRESSION_MULTIPLY_HEADER_GUARD

#include "rylang/data/expression.hpp"
#include <compare>

namespace rylang
{
    struct expression_multiply
    {
        static constexpr const char * const symbol = "*";
        static constexpr const char * const name = "multiply";
        static constexpr const int priority = 5;
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_multiply& other) const = default;
    };

    struct expression_divide
    {
        static constexpr const char * const symbol = "/";
        static constexpr const char * const name = "divide";
        static constexpr const int priority = 3;
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_divide& other) const = default;
    };

    struct expression_modulus
    {
        expression lhs;
        expression rhs;
        std::strong_ordering operator<=>(const expression_modulus& other) const = default;
    };
} // namespace rylang

#endif // RYLANG_EXPRESSION_MULTIPLY_HEADER_GUARD
