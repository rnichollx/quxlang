// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/static_test_vmir_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/data/compilation_result.hpp>

rpnx::querygraph::coroutine< quxlang::static_test_vmir_spec > quxlang::static_test_vmir_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_test >(sym) || !(co_await rpnx::querygraph::request< test_is_enabled_for_static_testing_query >(input)))
    {
        throw compiler_bug("static_test_vmir received a symbol that is not a static test: " + to_string(input));
    }

    // Compile the body in this phase so compile-time evaluation errors retain test failure semantics.
    instanciation_reference body{
        .temploid = temploid_reference{.templexoid = subsymbol{.of = input, .name = "__TEST_BODY"}, .overload_id = 0},
    };
    co_await rpnx::querygraph::request< user_vm_procedure3_query >(body);
    machine_target_info machine = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< static_test_vmir_spec > > generator(machine, input);
    co_return co_await generator.co_generate_test();
}
