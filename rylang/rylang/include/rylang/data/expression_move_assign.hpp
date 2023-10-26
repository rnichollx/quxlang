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
      expression lhs;
      expression rhs;
   };
}

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_MOVE_ASSIGN_HEADER
