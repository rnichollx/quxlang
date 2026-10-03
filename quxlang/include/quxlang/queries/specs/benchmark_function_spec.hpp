// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_BENCHMARK_FUNCTION_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_BENCHMARK_FUNCTION_HEADER_GUARD
#include <quxlang/queries/benchmark_function.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/vm_procedure3.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Registers dependencies of benchmark function specialization. */
    struct benchmark_function_spec
    {
        using query = benchmark_function_query;
        using dependencies = rpnx::typelist< symboid_query, lookup_query, instanciation_query, vm_procedure3_query >;
    };
    /** Specializes a benchmark for one suite case or standalone invocation. */
    rpnx::querygraph::coroutine< benchmark_function_spec > benchmark_function_impl(benchmark_function_input input);
} // namespace quxlang
#endif // QUXLANG_QUERIES_SPECS_BENCHMARK_FUNCTION_HEADER_GUARD
