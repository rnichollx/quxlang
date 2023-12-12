//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RYLANG_CONTEXTUAL_TYPE_REFERENCE_HEADER_GUARD
#define RYLANG_CONTEXTUAL_TYPE_REFERENCE_HEADER_GUARD

#include "lookup_chain.hpp"
#include "rylang/data/lookup_type.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
  struct contextual_type_reference
  {
      type_symbol context;
      type_symbol type;

      inline bool operator <(contextual_type_reference const& other) const
      {
          return std::tie(context, type) < std::tie(other.context, other.type);
      }

  };
}

#endif // RYLANG_CONTEXTUAL_TYPE_REFERENCE_HEADER_GUARD
