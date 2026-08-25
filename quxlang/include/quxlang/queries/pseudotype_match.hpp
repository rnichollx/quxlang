// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_PSEUDOTYPE_MATCH_HEADER_GUARD
#define QUXLANG_QUERIES_PSEUDOTYPE_MATCH_HEADER_GUARD

#include <quxlang/data/pseudotype_match.hpp>

#include <optional>

namespace quxlang
{
    /** Matches a pseudotype exactly against an already-canonical candidate. */
    struct pseudotype_match_query
    {
        static constexpr auto query_id = "pseudotype_match";
        using input_type = pseudotype_match_input;
        using output_type = std::optional< pseudotype_match_result >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_PSEUDOTYPE_MATCH_HEADER_GUARD
