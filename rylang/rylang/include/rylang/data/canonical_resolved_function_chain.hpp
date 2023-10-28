//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER
#define RPNX_RYANSCRIPT1031_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER

#include "canonical_lookup_chain.hpp"

namespace rylang
{
    struct canonical_resolved_function_chain
    {
        canonical_lookup_chain function_entity_chain;
        std::size_t overload_index;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER
