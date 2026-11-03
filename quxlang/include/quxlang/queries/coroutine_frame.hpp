// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_COROUTINE_FRAME_HEADER_GUARD
#define QUXLANG_QUERIES_COROUTINE_FRAME_HEADER_GUARD

#include <quxlang/data/coroutine_frame_info.hpp>

namespace quxlang
{
    /** Resolves the canonical frame layout for one concrete coroutine/controller specialization. */
    struct coroutine_frame_query
    {
        static constexpr auto query_id = "coroutine_frame";
        using input_type = instanciation_reference;
        using output_type = coroutine_frame_info;
    };
}

#endif // QUXLANG_QUERIES_COROUTINE_FRAME_HEADER_GUARD
