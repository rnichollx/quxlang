// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_GLOBAL_IS_PER_THREAD_HEADER_GUARD
#define QUXLANG_QUERIES_GLOBAL_IS_PER_THREAD_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /** Reports whether a global variable declaration uses PER_THREAD storage. */
    struct global_is_per_thread_query
    {
        static constexpr auto query_id = "global_is_per_thread";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_GLOBAL_IS_PER_THREAD_HEADER_GUARD
