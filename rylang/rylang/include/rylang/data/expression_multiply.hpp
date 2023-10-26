//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_MULTIPLY_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_MULTIPLY_HEADER

#include "rylang/data/expression.hpp"

namespace rylang
{
    struct expression_multiply
    {
        static constexpr const char * const symbol = "*";
        static constexpr const char * const name = "multiply";
        static constexpr const int priority = 5;
        expression lhs;
        expression rhs;
    };

    struct expression_divide
    {
        static constexpr const char * const symbol = "/";
        static constexpr const char * const name = "divide";
        static constexpr const int priority = 3;
        expression lhs;
        expression rhs;
    };

    struct expression_modulus
    {
        expression lhs;
        expression rhs;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_MULTIPLY_HEADER
