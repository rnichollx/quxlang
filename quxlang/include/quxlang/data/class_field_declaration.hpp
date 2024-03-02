//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef QUXLANG_CLASS_FIELD_DECLARATION_HEADER_GUARD
#define QUXLANG_CLASS_FIELD_DECLARATION_HEADER_GUARD

#include "contextual_type_reference.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"
namespace quxlang
{
  struct class_field_declaration
    {
      std::string name;
      type_symbol type;
  };

}

#endif // QUXLANG_CLASS_FIELD_DECLARATION_HEADER_GUARD
