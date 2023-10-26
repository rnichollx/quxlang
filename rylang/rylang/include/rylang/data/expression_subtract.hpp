//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_SUBTRACT_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_SUBTRACT_HEADER

#include "expression.hpp"

namespace rylang
{
    struct expression_subtract
    {
        expression lhs;
        expression rhs;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_SUBTRACT_HEADER
