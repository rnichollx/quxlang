//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_LOOKUP_SINGULAR_HEADER
#define RPNX_RYANSCRIPT1031_LOOKUP_SINGULAR_HEADER

#include <string>
#include "rylang/data/lookup_type.hpp"


namespace rylang
{
    struct lookup_singular
    {
        lookup_type type;
        std::string identifier;

        bool operator==(lookup_singular const& other) const
        {
            return std::tie(type, identifier) == std::tie(other.type, other.identifier);
        }

        bool operator!=(lookup_singular const& other) const
        {
            return std::tie(type, identifier) != std::tie(other.type, other.identifier);
        }

        bool operator <(lookup_singular const& other) const
        {
            return std::tie(type, identifier) < std::tie(other.type, other.identifier);
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_LOOKUP_SINGULAR_HEADER
