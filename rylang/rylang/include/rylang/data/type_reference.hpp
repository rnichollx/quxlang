//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_REFERENCE_HEADER

#include <boost/variant.hpp>
#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    struct pointer_reference;
    using type_reference = boost::variant< lookup_chain, boost::recursive_wrapper<pointer_reference> >;
} // namespace rylang

#include "pointer_reference.hpp"

#endif // RPNX_RYANSCRIPT1031_TYPE_REFERENCE_HEADER
