// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER_GUARD
#define QUXLANG_DATA_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER_GUARD

#include "type_symbol.hpp"

namespace quxlang
{
    struct [[deprecated("Use qualname")]] canonical_resolved_function_chain
    {
        type_symbol function_entity_chain;
        std::size_t overload_index;
    };
} // namespace quxlang

#endif // QUXLANG_CANONICAL_RESOLVED_FUNCTION_CHAIN_HEADER_GUARD
