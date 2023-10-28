//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CANONICAL_LOOKUP_CHAIN_HEADER
#define RPNX_RYANSCRIPT1031_CANONICAL_LOOKUP_CHAIN_HEADER

#include <string>
#include <vector>

namespace rylang
{
    struct canonical_lookup_chain : std::vector< std::string >
    {

        canonical_lookup_chain() = default;
        template < typename... Ts >
        explicit canonical_lookup_chain(Ts... ts)
            : std::vector< std::string >
        {
            ts...
        }
        {
        }

        canonical_lookup_chain(canonical_lookup_chain const&) = default;

        bool operator < (canonical_lookup_chain const& other) const
        {
            return static_cast< std::vector< std::string > const& >(*this) < static_cast< std::vector< std::string > const& >(other);
        }

        bool operator == (canonical_lookup_chain const& other) const
        {
            return static_cast< std::vector< std::string > const& >(*this) == static_cast< std::vector< std::string > const& >(other);
        }
        bool operator != (canonical_lookup_chain const& other) const
        {
            return static_cast< std::vector< std::string > const& >(*this) != static_cast< std::vector< std::string > const& >(other);
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CANONICAL_LOOKUP_CHAIN_HEADER
