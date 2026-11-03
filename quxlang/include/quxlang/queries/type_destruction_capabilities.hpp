// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_DESTRUCTION_CAPABILITIES_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_DESTRUCTION_CAPABILITIES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /** Records the destruction capabilities of a complete owning type graph. */
    struct type_destruction_capabilities
    {
        /** True when synchronous destruction cannot complete all owned lifetimes. */
        bool is_async_only = false;
        /** True when coroutine contexts select asynchronous destruction in the owning graph. */
        bool prefers_coroutine = false;

        RPNX_MEMBER_METADATA(type_destruction_capabilities, is_async_only, prefers_coroutine);
    };

    /** Classifies synchronous availability and coroutine preference throughout owned subobjects. */
    struct type_destruction_capabilities_query
    {
        static constexpr auto query_id = "type_destruction_capabilities";
        using input_type = type_symbol;
        using output_type = type_destruction_capabilities;
    };
}

#endif
