// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_IS_ASYNC_ONLY_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_IS_ASYNC_ONLY_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /** Reports whether a type requires asynchronous destruction of itself or an owned subobject. */
    struct type_is_async_only_query
    {
        static constexpr auto query_id = "type_is_async_only";
        using input_type = type_symbol;
        using output_type = bool;
    };
}

#endif
