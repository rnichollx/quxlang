// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_LIST_HEADER_GUARD
#define QUXLANG_DATA_CLASS_LIST_HEADER_GUARD
#include <vector>
#include <string>

namespace quxlang
{
  struct class_list
  {
    std::vector< std::string > class_names;
  };
}

#endif // QUXLANG_CLASS_LIST_HEADER_GUARD
