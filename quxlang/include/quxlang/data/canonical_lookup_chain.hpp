//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef QUXLANG_DATA_CANONICAL_LOOKUP_CHAIN_HEADER_GUARD
#define QUXLANG_DATA_CANONICAL_LOOKUP_CHAIN_HEADER_GUARD

#include <string>
#include <vector>

namespace quxlang
{
    struct [[deprecated("Use qualified_symbol_reference")]] canonical_lookup_chain : std::vector< std::string >
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
} // namespace quxlang

#endif // QUXLANG_CANONICAL_LOOKUP_CHAIN_HEADER_GUARD
