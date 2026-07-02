// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEST_IS_ENABLED_FOR_STATIC_TESTING_HEADER_GUARD
#define QUXLANG_QUERIES_TEST_IS_ENABLED_FOR_STATIC_TESTING_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /**
     * Returns whether a test declaration participates in static-test execution.
     */
    struct test_is_enabled_for_static_testing_query
    {
        static constexpr auto query_id = "test_is_enabled_for_static_testing";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEST_IS_ENABLED_FOR_STATIC_TESTING_HEADER_GUARD
