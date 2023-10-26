//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_ADD_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_ADD_HEADER

#include "rylang/data/expression.hpp"
#include "rylang/cow.hpp"

namespace rylang
{
    struct expression_add
    {
        static constexpr const char * const symbol = "+";
        static constexpr const char * const name = "add";
        static constexpr const int priority = 4;

        expression lhs;
        expression rhs;
    };

    struct expression_addp
    {
        expression lhs;
        expression rhs;
    };

    struct expression_addw
    {

    };
}

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_ADD_HEADER
