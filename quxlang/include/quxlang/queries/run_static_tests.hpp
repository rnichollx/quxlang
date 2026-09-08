// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD
#define QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD

#include <variant>
#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /** Counts completed and excluded compile-time tests. */
    struct static_test_results
    {
        std::size_t passed = 0;
        std::size_t known_broken = 0;
        RPNX_MEMBER_METADATA(static_test_results, passed, known_broken);
    };

    /** Executes enabled compile-time tests and returns their totals. */
    struct run_static_tests_query
    {
        static constexpr auto query_id = "run_static_tests";
        using input_type = std::monostate;
        using output_type = static_test_results;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD
