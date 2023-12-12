//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RYLANG_LOOKUP_SINGULAR_HEADER_GUARD
#define RYLANG_LOOKUP_SINGULAR_HEADER_GUARD

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

#endif // RYLANG_LOOKUP_SINGULAR_HEADER_GUARD
