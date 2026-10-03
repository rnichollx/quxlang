// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_BENCHMARK_ENTRIES_HEADER_GUARD
#define QUXLANG_QUERIES_BENCHMARK_ENTRIES_HEADER_GUARD
#include <quxlang/data/benchmark.hpp>
namespace quxlang
{
    /** Expands benchmark cases from the explicitly selected discovery roots. */
    struct benchmark_entries_query
    {
        static constexpr auto query_id = "benchmark_entries";
        using input_type = std::vector< std::string >;
        using output_type = std::vector< benchmark_entry >;
    };
} // namespace quxlang
#endif // QUXLANG_QUERIES_BENCHMARK_ENTRIES_HEADER_GUARD
