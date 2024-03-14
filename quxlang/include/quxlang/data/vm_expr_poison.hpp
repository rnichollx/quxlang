//
// Created by Ryan Nicholl on 11/20/23.
//

#ifndef QUXLANG_VM_EXPR_POISON_HEADER_GUARD
#define QUXLANG_VM_EXPR_POISON_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
  struct vm_expr_poison
  {
      type_symbol type;

      std::strong_ordering operator<=>(const vm_expr_poison& other) const
      {
          return rpnx::compare(type, other.type);
      }
  };
}

#endif // QUXLANG_VM_EXPR_POISON_HEADER_GUARD
