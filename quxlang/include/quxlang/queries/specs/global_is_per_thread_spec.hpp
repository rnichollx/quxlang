// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_GLOBAL_IS_PER_THREAD_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_GLOBAL_IS_PER_THREAD_SPEC_HEADER_GUARD

#include <quxlang/queries/global_is_per_thread.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler spec for PER_THREAD global classification. */
    struct global_is_per_thread_spec
    {
        using query = global_is_per_thread_query;
        using dependencies = rpnx::typelist< symboid_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< global_is_per_thread_spec > global_is_per_thread_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_GLOBAL_IS_PER_THREAD_SPEC_HEADER_GUARD
