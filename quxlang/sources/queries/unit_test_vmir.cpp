// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/unit_test_vmir_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/data/compilation_result.hpp>

rpnx::querygraph::coroutine< quxlang::unit_test_vmir_spec > quxlang::unit_test_vmir_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_test >(sym) || !(co_await rpnx::querygraph::request< test_is_enabled_for_unit_testing_query >(input)))
    {
        throw compiler_bug("unit_test_vmir received a symbol that is not a unit test: " + to_string(input));
    }

    auto const& test = as< ast2_test >(sym);
    auto machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< unit_test_vmir_spec > > gen(machine_info, input);
    co_return co_await gen.co_generate_unit_test(test);
}
