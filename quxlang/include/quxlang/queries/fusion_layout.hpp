// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUSION_LAYOUT_HEADER_GUARD
#define QUXLANG_QUERIES_FUSION_LAYOUT_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/fusion_layout.hpp>

namespace quxlang
{
    /// Returns the target-specific storage layout for a UNION or VARIANT type.
    struct fusion_layout_query
    {
        static constexpr auto query_id = "fusion_layout";
        using input_type = type_symbol;
        using output_type = fusion_layout;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUSION_LAYOUT_HEADER_GUARD
