// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CANONICAL_LOOKUP_HEADER_GUARD
#define QUXLANG_QUERIES_CANONICAL_LOOKUP_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/contextual_type_reference.hpp>

#include <optional>

namespace quxlang
{
    /** Canonicalizes a type reference without applying source access control. */
    struct canonical_lookup_query
    {
        static constexpr auto query_id = "canonical_lookup";
        using input_type = contextual_type_reference;
        using output_type = std::optional< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CANONICAL_LOOKUP_HEADER_GUARD
