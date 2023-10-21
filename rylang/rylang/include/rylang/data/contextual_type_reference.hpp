//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CONTEXTUAL_TYPE_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_CONTEXTUAL_TYPE_REFERENCE_HEADER

#include "lookup_chain.hpp"
#include "rylang/ast/type_ref_ast.hpp"
#include "rylang/data/lookup_type.hpp"

namespace rylang
{
  struct contextual_type_reference
  {
      canonical_lookup_chain context;
      type_reference type;

      inline bool operator <(contextual_type_reference const& other) const
      {
          return std::tie(context, type) < std::tie(other.context, other.type);
      }

  };
}

#endif // RPNX_RYANSCRIPT1031_CONTEXTUAL_TYPE_REFERENCE_HEADER
