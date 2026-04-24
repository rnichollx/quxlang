// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_GLOBAL_IS_SERIALOID_STATIC_HEADER_GUARD
#define QUXLANG_QUERIES_GLOBAL_IS_SERIALOID_STATIC_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct global_is_serialoid_static_query
    {
        static constexpr auto query_id = "global_is_serialoid_static";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_GLOBAL_IS_SERIALOID_STATIC_HEADER_GUARD
