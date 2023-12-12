//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RYLANG_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER_GUARD
#define RYLANG_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER_GUARD

#include "qualified_symbol_reference.hpp"

namespace rylang
{
    struct [[deprecated("Use qualname")]] canonical_resolved_function_chain
    {
        type_symbol function_entity_chain;
        std::size_t overload_index;
    };
} // namespace rylang

#endif // RYLANG_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER_GUARD
