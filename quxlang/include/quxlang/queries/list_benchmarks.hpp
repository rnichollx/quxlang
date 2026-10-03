// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LIST_BENCHMARKS_HEADER_GUARD
#define QUXLANG_QUERIES_LIST_BENCHMARKS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <set>

namespace quxlang
{
    /**
     * Discovers BENCHMARK declarations reachable from one module, namespace, or class.
     */
    struct list_benchmarks_query
    {
        static constexpr auto query_id = "list_benchmarks";
        using input_type = type_symbol;
        using output_type = std::set< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LIST_BENCHMARKS_HEADER_GUARD
