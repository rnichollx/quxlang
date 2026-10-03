// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_BENCHMARK_ENTRIES_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_BENCHMARK_ENTRIES_HEADER_GUARD
#include <quxlang/queries/benchmark_entries.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/list_benchmarks.hpp>
#include <quxlang/queries/symboid.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Registers dependencies for benchmark suite case discovery. */
    struct benchmark_entries_spec
    {
        using query = benchmark_entries_query;
        using dependencies = rpnx::typelist< list_benchmarks_query, symboid_query, instanciation_query >;
    };
    /** Resolves stable names and canonical routine identities for selected cases. */
    rpnx::querygraph::coroutine< benchmark_entries_spec > benchmark_entries_impl(std::vector< std::string > input);
} // namespace quxlang
#endif // QUXLANG_QUERIES_SPECS_BENCHMARK_ENTRIES_HEADER_GUARD
