//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_LOOKUP_CHAIN_HEADER
#define RPNX_RYANSCRIPT1031_LOOKUP_CHAIN_HEADER

#include "rylang/data/lookup_singular.hpp"
#include <vector>

namespace rylang
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
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_LOOKUP_CHAIN_HEADER
