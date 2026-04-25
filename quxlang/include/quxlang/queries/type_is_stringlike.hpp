// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_IS_STRINGLIKE_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_IS_STRINGLIKE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct type_is_stringlike_query
    {
        static constexpr auto query_id = "type_is_stringlike";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TYPE_IS_STRINGLIKE_HEADER_GUARD
