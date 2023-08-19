//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_HEADER

#include <cstdint>
#include <string>

#include "rylang/data/symbol_id.hpp"

namespace rylang
{
  struct class_member_variable
  {
    std::uint64_t class_id;
    std::string name;
  };
}

#endif // RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_HEADER
