//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef QUXLANG_DATA_CONTEXTUAL_TYPE_REFERENCE_HEADER_GUARD
#define QUXLANG_DATA_CONTEXTUAL_TYPE_REFERENCE_HEADER_GUARD

#include "lookup_chain.hpp"
#include "quxlang/data/lookup_type.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
  struct contextual_type_reference
  {
      type_symbol context;
      type_symbol type;

      RPNX_MEMBER_METADATA(contextual_type_reference, context, type);

  };
}

#endif // QUXLANG_CONTEXTUAL_TYPE_REFERENCE_HEADER_GUARD
