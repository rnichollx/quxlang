// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_UNIT_TEST_VMIR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_UNIT_TEST_VMIR_SPEC_HEADER_GUARD

#include <quxlang/co_vmir_generator2_deps.hpp>
#include <quxlang/queries/test_is_enabled_for_unit_testing.hpp>
#include <quxlang/queries/unit_test_vmir.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct unit_test_vmir_spec
    {
        using query = unit_test_vmir_query;
        using dependencies = typename append_co_vmir_generator2_dependencies< co_vmir_generator2_query_deps, test_is_enabled_for_unit_testing_query >::type;
    };

    rpnx::querygraph::coroutine< unit_test_vmir_spec > unit_test_vmir_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_UNIT_TEST_VMIR_SPEC_HEADER_GUARD
