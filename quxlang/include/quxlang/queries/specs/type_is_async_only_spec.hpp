// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TYPE_IS_ASYNC_ONLY_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TYPE_IS_ASYNC_ONLY_HEADER_GUARD

#include <quxlang/queries/type_destruction_capabilities.hpp>
#include <quxlang/queries/type_is_async_only.hpp>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Declares the semantic capability dependency of the language trait. */
    struct type_is_async_only_spec
    {
        using query = type_is_async_only_query;
        using dependencies = rpnx::typelist< type_destruction_capabilities_query >;
    };

    /** Projects synchronous destruction availability into the TYPE_IS_ASYNC_ONLY language trait. */
    rpnx::querygraph::coroutine< type_is_async_only_spec > type_is_async_only_impl(type_symbol input);
}

#endif
