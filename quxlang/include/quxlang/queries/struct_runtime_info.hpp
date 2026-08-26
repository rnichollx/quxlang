// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_RUNTIME_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_RUNTIME_INFO_HEADER_GUARD

#include <quxlang/data/struct_inheritance.hpp>

namespace quxlang
{
    /** Returns backend-ready runtime descriptors for one complete struct type. */
    struct struct_runtime_info_query
    {
        static constexpr auto query_id = "struct_runtime_info";
        using input_type = type_symbol;
        using output_type = struct_runtime_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_RUNTIME_INFO_HEADER_GUARD
