//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RYLANG_CLASS_FIELD_DECLARATION_HEADER_GUARD
#define RYLANG_CLASS_FIELD_DECLARATION_HEADER_GUARD

#include "contextual_type_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
namespace rylang
{
  struct class_field_declaration
    {
      std::string name;
      type_symbol type;
  };

}

#endif // RYLANG_CLASS_FIELD_DECLARATION_HEADER_GUARD
