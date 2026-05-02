// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_RUN_STATIC_TESTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_RUN_STATIC_TESTS_SPEC_HEADER_GUARD

#include <quxlang/queries/list_static_tests.hpp>
#include <quxlang/queries/run_static_test.hpp>
#include <quxlang/queries/run_static_tests.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct run_static_tests_spec
    {
        using query = run_static_tests_query;
        using dependencies = rpnx::typelist< list_static_tests_query, run_static_test_query >;
    };

    rpnx::querygraph::coroutine< run_static_tests_spec > run_static_tests_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_RUN_STATIC_TESTS_SPEC_HEADER_GUARD
