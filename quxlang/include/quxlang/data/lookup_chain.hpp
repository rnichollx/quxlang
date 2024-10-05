// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_LOOKUP_CHAIN_HEADER_GUARD
#define QUXLANG_DATA_LOOKUP_CHAIN_HEADER_GUARD

#include "quxlang/data/lookup_singular.hpp"
#include <vector>

namespace quxlang
{
    template < typename Alloc >
    struct basic_lookup_chain
    {
        std::vector< lookup_singular, Alloc > chain;

        bool operator==(basic_lookup_chain const& other) const
        {
            return chain == other.chain;
        }

        bool operator!=(basic_lookup_chain const& other) const
        {
            return chain != other.chain;
        }

        bool operator <(basic_lookup_chain const& other) const
        {
            return chain < other.chain;
        }
    };

    typedef basic_lookup_chain< std::allocator< lookup_singular > > lookup_chain;
} // namespace quxlang

#endif // QUXLANG_LOOKUP_CHAIN_HEADER_GUARD
