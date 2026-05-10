// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ENUM_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_ENUM_INFO_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/enum_flagset_info.hpp>

namespace quxlang
{
    struct enum_info_query
    {
        static constexpr auto query_id = "enum_info";
        using input_type = type_symbol;
        using output_type = enum_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ENUM_INFO_HEADER_GUARD
