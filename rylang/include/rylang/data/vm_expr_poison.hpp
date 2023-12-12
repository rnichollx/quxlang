//
// Created by Ryan Nicholl on 11/20/23.
//

#ifndef RYLANG_VM_EXPR_POISON_HEADER_GUARD
#define RYLANG_VM_EXPR_POISON_HEADER_GUARD

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
  struct vm_expr_poison
  {
      type_symbol type;
  };
}

#endif // RYLANG_VM_EXPR_POISON_HEADER_GUARD
