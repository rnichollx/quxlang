// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STATIC_TEST_VMIR_HEADER_GUARD
#define QUXLANG_QUERIES_STATIC_TEST_VMIR_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang
{
    /**
     * Generates the VMIR2 routine for one STATIC_TEST symbol without executing it.
     */
    struct static_test_vmir_query
    {
        static constexpr auto query_id = "static_test_vmir";
        using input_type = type_symbol;
        using output_type = vmir2::functanoid_routine3;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STATIC_TEST_VMIR_HEADER_GUARD
