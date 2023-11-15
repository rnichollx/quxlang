//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CONTEXTUAL_TYPE_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_CONTEXTUAL_TYPE_REFERENCE_HEADER

#include "lookup_chain.hpp"
#include "rylang/data/lookup_type.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
  struct contextual_type_reference
  {
      qualified_symbol_reference context;
      qualified_symbol_reference type;

      inline bool operator <(contextual_type_reference const& other) const
      {
          return std::tie(context, type) < std::tie(other.context, other.type);
      }

  };
}

#endif // RPNX_RYANSCRIPT1031_CONTEXTUAL_TYPE_REFERENCE_HEADER
