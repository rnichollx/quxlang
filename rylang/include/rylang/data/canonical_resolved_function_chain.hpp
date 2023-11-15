//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER
#define RPNX_RYANSCRIPT1031_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER

#include "qualified_symbol_reference.hpp"

namespace rylang
{
    struct [[deprecated("Use qualname")]] canonical_resolved_function_chain
    {
        qualified_symbol_reference function_entity_chain;
        std::size_t overload_index;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER
