//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_FIELD_DECLARATION_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_FIELD_DECLARATION_HEADER

#include "contextual_type_reference.hpp"
#include "type_reference.hpp"
namespace rylang
{
  struct class_field_declaration
    {
      std::string name;
      type_reference type;
  };

}

#endif // RPNX_RYANSCRIPT1031_CLASS_FIELD_DECLARATION_HEADER
