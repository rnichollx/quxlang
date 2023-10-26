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
        expression lhs;
        expression rhs;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_MULTIPLY_HEADER
