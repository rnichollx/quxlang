//
// Created by Ryan Nicholl on 11/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_EXPR_POISON_HEADER
#define RPNX_RYANSCRIPT1031_VM_EXPR_POISON_HEADER

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
  struct vm_expr_poison
  {
      qualified_symbol_reference type;
  };
}

#endif // RPNX_RYANSCRIPT1031_VM_EXPR_POISON_HEADER
