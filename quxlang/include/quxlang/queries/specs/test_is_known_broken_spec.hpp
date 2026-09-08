// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_TEST_IS_KNOWN_BROKEN_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEST_IS_KNOWN_BROKEN_HEADER_GUARD
#include <quxlang/queries/test_is_known_broken.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Dependencies required to evaluate test exclusion conditions. */
    struct test_is_known_broken_spec
    {
        using query = test_is_known_broken_query;
        using dependencies = rpnx::typelist< symboid_query, constexpr_bool_query >;
    };
    /** Evaluates a test exclusion condition without compiling its body. */
    rpnx::querygraph::coroutine< test_is_known_broken_spec > test_is_known_broken_impl(type_symbol input);
}
#endif
