// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_UNION_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_UNION_INFO_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/fusion_info.hpp>

namespace quxlang
{
    /// Returns normalized semantic information for a UNION type symbol.
    struct union_info_query
    {
        static constexpr auto query_id = "union_info";
        using input_type = type_symbol;
        using output_type = union_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_UNION_INFO_HEADER_GUARD
