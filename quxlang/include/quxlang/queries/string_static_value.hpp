// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRING_STATIC_VALUE_HEADER_GUARD
#define QUXLANG_QUERIES_STRING_STATIC_VALUE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct string_static_value_query
    {
        static constexpr auto query_id = "string_static_value";
        using input_type = type_symbol;
        using output_type = constexpr_string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRING_STATIC_VALUE_HEADER_GUARD
