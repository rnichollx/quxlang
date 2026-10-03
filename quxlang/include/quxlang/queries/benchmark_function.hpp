// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_BENCHMARK_FUNCTION_HEADER_GUARD
#define QUXLANG_QUERIES_BENCHMARK_FUNCTION_HEADER_GUARD
#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    /** Identifies one benchmark specialization, including its measurement mode. */
    struct benchmark_function_input
    {
        type_symbol benchmark;
        bool standalone = false;
        std::uint64_t case_index = 0;
        RPNX_MEMBER_METADATA(benchmark_function_input, benchmark, standalone, case_index);
    };

    /** Produces an ordinary function declaration for one benchmark specialization. */
    struct benchmark_function_query
    {
        static constexpr auto query_id = "benchmark_function";
        using input_type = benchmark_function_input;
        using output_type = ast2_function_declaration;
    };
} // namespace quxlang
#endif // QUXLANG_QUERIES_BENCHMARK_FUNCTION_HEADER_GUARD
