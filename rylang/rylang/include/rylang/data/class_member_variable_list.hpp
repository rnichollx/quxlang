//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_HEADER

#include <vector>
#include "rylang/data/class_member_variable.hpp"

namespace rylang
{
  struct class_member_variable_list
  {
    std::vector< class_member_variable > member_variables;
  };
}

#endif // RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_LIST_HEADER
