//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_COPY_ASSIGN_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_COPY_ASSIGN_HEADER

#include "expression.hpp"

namespace rylang
{
    struct expression_copy_assign
    {
        static constexpr const char* name = "copy_assign";
        static constexpr const char* symbol = ":=";
        static constexpr const int priority = 0;
        expression lhs;
        expression rhs;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_COPY_ASSIGN_HEADER
