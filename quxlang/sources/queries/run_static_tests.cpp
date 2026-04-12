// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/run_static_tests_spec.hpp>

rpnx::querygraph::coroutine< quxlang::run_static_tests_spec > quxlang::run_static_tests_impl(type_symbol input)
{
    auto tests = co_await rpnx::querygraph::request< list_static_tests_query >(input);

    for (type_symbol const& test : tests)
    {
        co_await rpnx::querygraph::request< run_static_test_query >(test);
    }

    co_return true;
}
