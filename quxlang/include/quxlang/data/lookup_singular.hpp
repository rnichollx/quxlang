// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_LOOKUP_SINGULAR_HEADER_GUARD
#define QUXLANG_DATA_LOOKUP_SINGULAR_HEADER_GUARD

#include <string>
#include "quxlang/data/lookup_type.hpp"


namespace quxlang
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
} // namespace quxlang

#endif // QUXLANG_LOOKUP_SINGULAR_HEADER_GUARD
