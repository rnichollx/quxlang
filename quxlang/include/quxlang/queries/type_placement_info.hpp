// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_PLACEMENT_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_PLACEMENT_INFO_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/type_placement_info.hpp>


namespace quxlang
{
    struct type_placement_info_query
    {
        static constexpr auto query_id = "type_placement_info";
        using input_type = type_symbol;
        using output_type = type_placement_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TYPE_PLACEMENT_INFO_HEADER_GUARD
