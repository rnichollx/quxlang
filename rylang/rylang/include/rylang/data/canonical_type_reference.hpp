//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANONICAL_TYPE_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_CANONICAL_TYPE_REFERENCE_HEADER

#include <boost/variant.hpp>

#include "canonical_lookup_chain.hpp"
#include "rylang/ast/integral_keyword_ast.hpp"

namespace rylang
{
   struct canonical_pointer_type_reference;

   using canonical_type_reference = boost::variant< canonical_lookup_chain, boost::recursive_wrapper<canonical_pointer_type_reference>, integral_keyword_ast >;

}

#include "canonical_pointer_type_reference.hpp"

#endif // RPNX_RYANSCRIPT1031_CANONICAL_TYPE_REFERENCE_HEADER
