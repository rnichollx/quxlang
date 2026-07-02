// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/test_is_enabled_for_static_testing_spec.hpp>

rpnx::querygraph::coroutine< quxlang::test_is_enabled_for_static_testing_spec > quxlang::test_is_enabled_for_static_testing_impl(type_symbol input)
{
    ast2_symboid const sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_test >(sym))
    {
        throw compiler_bug("test_is_enabled_for_static_testing received a symbol that is not a test: " + to_string(input));
    }

    ast2_test const& test = as< ast2_test >(sym);
    co_return test.mode == ast2_test_mode::static_only || test.mode == ast2_test_mode::dual;
}
