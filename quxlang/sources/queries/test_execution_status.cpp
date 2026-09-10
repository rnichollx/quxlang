// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/test_execution_status_spec.hpp>
#include <quxlang/data/compilation_result.hpp>

rpnx::querygraph::coroutine< quxlang::test_execution_status_spec > quxlang::test_execution_status_impl(type_symbol input)
{
    ast2_symboid const& symbol = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_test >(symbol))
    {
        throw compiler_bug("test_execution_status received a symbol that is not a test: " + to_string(input));
    }
    ast2_test const& test = as< ast2_test >(symbol);
    if (test.known_broken.has_value() && (co_await rpnx::querygraph::request< constexpr_bool_query >(constexpr_input{.expr = *test.known_broken, .context = input})))
    {
        co_return test_execution_status::known_broken;
    }
    if (test.known_failing.has_value() && (co_await rpnx::querygraph::request< constexpr_bool_query >(constexpr_input{.expr = *test.known_failing, .context = input})))
    {
        co_return test_execution_status::known_failing;
    }
    co_return test_execution_status::runnable;
}
