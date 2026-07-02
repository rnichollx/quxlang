// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEST_IS_ENABLED_FOR_UNIT_TESTING_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEST_IS_ENABLED_FOR_UNIT_TESTING_SPEC_HEADER_GUARD

#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/test_is_enabled_for_unit_testing.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct test_is_enabled_for_unit_testing_spec
    {
        using query = test_is_enabled_for_unit_testing_query;
        using dependencies = rpnx::typelist< symboid_query >;
    };

    /// Returns true for UNIT_TEST and DUAL_TEST declarations.
    rpnx::querygraph::coroutine< test_is_enabled_for_unit_testing_spec > test_is_enabled_for_unit_testing_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEST_IS_ENABLED_FOR_UNIT_TESTING_SPEC_HEADER_GUARD
