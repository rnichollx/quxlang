//
// Created by Ryan Nicholl on 11/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_EQUALS_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_EQUALS_HEADER

#include "expression.hpp"

namespace rylang
{
    struct expression_equals
    {
        static constexpr const char * name = "equals";
        static constexpr const char * symbol = "==";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_equals& other) const  = default;
    };

    struct expression_not_equals
    {
        static constexpr const char * name = "not_equals";
        static constexpr const char * symbol = "!=";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_not_equals& other) const  = default;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_EQUALS_HEADER
