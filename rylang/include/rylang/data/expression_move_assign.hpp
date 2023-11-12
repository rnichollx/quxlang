//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_MOVE_ASSIGN_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_MOVE_ASSIGN_HEADER

#include "expression.hpp"
namespace rylang
{
    struct expression_move_assign
    {
        static constexpr const char* name = "move_assign";
        static constexpr const char* symbol = ":<";
        static constexpr const int priority = 0;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_move_assign& other) const  = default;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_MOVE_ASSIGN_HEADER
