//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_S_EXPRESSION_ADD_HEADER
#define RPNX_RYANSCRIPT1031_S_EXPRESSION_ADD_HEADER

#include <memory>
#include "rylang/data/s_expression.hpp"

namespace rylang
{
   struct s_expression_add
   {
       s_expression_ptr lhs;
       s_expression_ptr rhs;
   };
}

#endif // RPNX_RYANSCRIPT1031_S_EXPRESSION_ADD_HEADER
