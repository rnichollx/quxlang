// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LIST_STATIC_TESTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LIST_STATIC_TESTS_SPEC_HEADER_GUARD

#include <quxlang/queries/list_static_tests.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/symboid_subdeclaroids.hpp>
#include <quxlang/queries/test_is_enabled_for_static_testing.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct list_static_tests_spec
    {
        using query = list_static_tests_query;
        using dependencies = rpnx::typelist< constexpr_bool_query, symboid_subdeclaroids_query, test_is_enabled_for_static_testing_query, list_static_tests_query >;
    };

    rpnx::querygraph::coroutine< list_static_tests_spec > list_static_tests_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LIST_STATIC_TESTS_SPEC_HEADER_GUARD
