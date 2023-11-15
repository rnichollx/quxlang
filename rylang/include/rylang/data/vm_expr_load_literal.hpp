//
// Created by Ryan Nicholl on 11/14/23.
//

#include "qualified_symbol_reference.hpp"

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_LOAD_LITERAL_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_LOAD_LITERAL_HEADER

#include <string>

namespace rylang
{
   struct vm_expr_load_literal
   {
       std::string literal;
       qualified_symbol_reference type;
   };
}

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_LOAD_LITERAL_HEADER
