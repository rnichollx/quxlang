//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_CALL_OVERLOAD_SET_HEADER
#define RPNX_RYANSCRIPT1031_CALL_OVERLOAD_SET_HEADER

#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/ordering.hpp"

namespace rylang
{
    struct call_overload_set
    {
        std::vector< qualified_symbol_reference > argument_types;

        std::strong_ordering operator<=>(const call_overload_set& other) const
        {
            return strong_ordering_from_less(argument_types, other.argument_types);
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CALL_OVERLOAD_SET_HEADER
