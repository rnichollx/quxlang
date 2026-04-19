// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ANTESTATAL_STATIC_VALUE_HEADER_GUARD
#define QUXLANG_QUERIES_ANTESTATAL_STATIC_VALUE_HEADER_GUARD

#include <quxlang/data/antestatal.hpp>
#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct antestatal_static_value_query
    {
        static constexpr auto query_id = "antestatal_static_value";
        using input_type = type_symbol;
        using output_type = antestatal_value;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ANTESTATAL_STATIC_VALUE_HEADER_GUARD
