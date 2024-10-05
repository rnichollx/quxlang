// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_VM_EXPR_POISON_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_POISON_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
  struct vm_expr_poison
  {
      type_symbol type;

      RPNX_MEMBER_METADATA(vm_expr_poison, type);
  };
}

#endif // QUXLANG_VM_EXPR_POISON_HEADER_GUARD
