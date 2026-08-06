// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUSION_ALTERNATIVES_LIST_HEADER_GUARD
#define QUXLANG_QUERIES_FUSION_ALTERNATIVES_LIST_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <vector>

namespace quxlang
{
    /** Returns the canonical alternative types of a UNION or VARIANT in declaration order. */
    struct fusion_alternatives_list_query
    {
        static constexpr auto query_id = "fusion_alternatives_list";
        using input_type = type_symbol;
        using output_type = std::vector< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUSION_ALTERNATIVES_LIST_HEADER_GUARD
