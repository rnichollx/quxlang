// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_TEST_EXECUTION_STATUS_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEST_EXECUTION_STATUS_HEADER_GUARD
#include <quxlang/queries/test_execution_status.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Dependencies required to evaluate test exclusion conditions. */
    struct test_execution_status_spec
    {
        using query = test_execution_status_query;
        using dependencies = rpnx::typelist< symboid_query, constexpr_bool_query >;
    };
    /** Evaluates a test exclusion condition without compiling its body. */
    rpnx::querygraph::coroutine< test_execution_status_spec > test_execution_status_impl(type_symbol input);
}
#endif
