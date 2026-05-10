// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FLAGSET_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_FLAGSET_INFO_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/enum_flagset_info.hpp>

namespace quxlang
{
    struct flagset_info_query
    {
        static constexpr auto query_id = "flagset_info";
        using input_type = type_symbol;
        using output_type = flagset_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FLAGSET_INFO_HEADER_GUARD
