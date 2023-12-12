//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RYLANG_CLASS_MEMBER_VARIABLE_PRECURSOR1_HEADER_GUARD
#define RYLANG_CLASS_MEMBER_VARIABLE_PRECURSOR1_HEADER_GUARD

#include <cstdint>
#include <string>

#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
  struct class_member_variable_precursor1
  {
      std::string name;
      lookup_chain chain;
  };
}

#endif // RYLANG_CLASS_MEMBER_VARIABLE_PRECURSOR1_HEADER_GUARD
