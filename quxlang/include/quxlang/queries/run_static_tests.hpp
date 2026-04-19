// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD
#define QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct run_static_tests_query
    {
        static constexpr auto query_id = "run_static_tests";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_RUN_STATIC_TESTS_HEADER_GUARD
