// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLOID_FORMAL_ENSIG_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLOID_FORMAL_ENSIG_HEADER_GUARD

#include <optional>

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /// Resolves the canonical formal ensig for a selected overload reference.
    struct temploid_formal_ensig_query
    {
        static constexpr auto query_id = "temploid_formal_ensig";
        using input_type = temploid_reference;
        using output_type = std::optional< temploid_ensig >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLOID_FORMAL_ENSIG_HEADER_GUARD
