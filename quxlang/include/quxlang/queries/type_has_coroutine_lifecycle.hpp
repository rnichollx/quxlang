// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_HAS_COROUTINE_LIFECYCLE_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_HAS_COROUTINE_LIFECYCLE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /** Reports whether an owning type graph declares a coroutine lifecycle operation. */
    struct type_has_coroutine_lifecycle_query
    {
        static constexpr auto query_id = "type_has_coroutine_lifecycle";
        using input_type = type_symbol;
        using output_type = bool;
    };
}

#endif
