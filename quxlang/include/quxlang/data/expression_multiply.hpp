//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef QUXLANG_DATA_EXPRESSION_MULTIPLY_HEADER_GUARD
#define QUXLANG_DATA_EXPRESSION_MULTIPLY_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include <compare>

namespace quxlang
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
} // namespace quxlang

#endif // QUXLANG_EXPRESSION_MULTIPLY_HEADER_GUARD
