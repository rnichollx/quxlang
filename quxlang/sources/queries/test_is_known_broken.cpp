// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/test_is_known_broken_spec.hpp>
#include <quxlang/data/compilation_result.hpp>

rpnx::querygraph::coroutine< quxlang::test_is_known_broken_spec > quxlang::test_is_known_broken_impl(type_symbol input)
{
    ast2_symboid const& symbol = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_test >(symbol))
    {
        throw compiler_bug("test_is_known_broken received a symbol that is not a test: " + to_string(input));
    }
    ast2_test const& test = as< ast2_test >(symbol);
    if (!test.known_broken.has_value())
    {
        co_return false;
    }
    co_return co_await rpnx::querygraph::request< constexpr_bool_query >(constexpr_input{
        .expr = *test.known_broken,
        .context = input,
    });
}
