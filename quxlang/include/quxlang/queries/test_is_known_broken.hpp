// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_TEST_IS_KNOWN_BROKEN_HEADER_GUARD
#define QUXLANG_QUERIES_TEST_IS_KNOWN_BROKEN_HEADER_GUARD
#include <quxlang/data/basic_types.hpp>
namespace quxlang
{
    /** Returns whether a test's compile-time known-broken condition is true. */
    struct test_is_known_broken_query
    {
        static constexpr auto query_id = "test_is_known_broken";
        using input_type = type_symbol;
        using output_type = bool;
    };
}
#endif
