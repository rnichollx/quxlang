// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_TEST_EXECUTION_STATUS_HEADER_GUARD
#define QUXLANG_QUERIES_TEST_EXECUTION_STATUS_HEADER_GUARD
#include <quxlang/data/basic_types.hpp>
#include <cstdint>
#include <rpnx/macros.hpp>
/** Controls whether a test can compile and whether execution is opt-in. */
RPNX_ENUM(quxlang, test_execution_status, std::uint8_t, runnable, known_broken, known_failing);

namespace quxlang
{
    /** Classifies a test after evaluating its compile-time skip condition. */
    struct test_execution_status_query
    {
        static constexpr auto query_id = "test_execution_status";
        using input_type = type_symbol;
        using output_type = test_execution_status;
    };
}
#endif
