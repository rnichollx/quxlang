// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD
#define QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD

#include <variant>

namespace quxlang
{
    struct run_static_tests_query
    {
        static constexpr auto query_id = "run_static_tests";
        using input_type = std::monostate;
        using output_type = std::monostate;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD
