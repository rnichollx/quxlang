// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEST_IS_ENABLED_FOR_UNIT_TESTING_HEADER_GUARD
#define QUXLANG_QUERIES_TEST_IS_ENABLED_FOR_UNIT_TESTING_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /**
     * Returns whether a test declaration participates in runtime unit-test execution.
     */
    struct test_is_enabled_for_unit_testing_query
    {
        static constexpr auto query_id = "test_is_enabled_for_unit_testing";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEST_IS_ENABLED_FOR_UNIT_TESTING_HEADER_GUARD
