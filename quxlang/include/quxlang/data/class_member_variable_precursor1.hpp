// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_MEMBER_VARIABLE_PRECURSOR1_HEADER_GUARD
#define QUXLANG_DATA_CLASS_MEMBER_VARIABLE_PRECURSOR1_HEADER_GUARD

#include <cstdint>
#include <string>

#include "quxlang/data/lookup_chain.hpp"

namespace quxlang
{
  struct class_member_variable_precursor1
  {
      std::string name;
      lookup_chain chain;
  };
}

#endif // QUXLANG_CLASS_MEMBER_VARIABLE_PRECURSOR1_HEADER_GUARD
