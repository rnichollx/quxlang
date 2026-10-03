// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LIST_BENCHMARKS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LIST_BENCHMARKS_SPEC_HEADER_GUARD

#include <quxlang/queries/active_symboid_subdeclaroids.hpp>
#include <quxlang/queries/list_benchmarks.hpp>
#include <quxlang/queries/symboid.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Registers the recursive benchmark discovery dependencies. */
    struct list_benchmarks_spec
    {
        using query = list_benchmarks_query;
        using dependencies = rpnx::typelist< active_symboid_subdeclaroids_query, list_benchmarks_query >;
    };

    /** Lists declarations under one explicit benchmark discovery root. */
    rpnx::querygraph::coroutine< list_benchmarks_spec > list_benchmarks_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LIST_BENCHMARKS_SPEC_HEADER_GUARD
