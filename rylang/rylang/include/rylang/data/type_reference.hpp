//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_REFERENCE_HEADER

#include "absolute_lookup_reference.hpp"
#include "proximate_lookup_reference.hpp"
#include "rylang/data/lookup_chain.hpp"
#include <boost/variant.hpp>

namespace rylang
{
    struct pointer_reference;
    using type_reference = boost::variant< absolute_lookup_reference, proximate_lookup_reference, integral_keyword_ast, boost::recursive_wrapper<pointer_reference> >;
} // namespace rylang

#include "pointer_reference.hpp"

#endif // RPNX_RYANSCRIPT1031_TYPE_REFERENCE_HEADER
