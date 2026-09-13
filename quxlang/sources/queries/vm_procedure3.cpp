// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/vm_procedure3_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>

#include "quxlang/manipulators/typeutils.hpp"




rpnx::querygraph::coroutine< quxlang::vm_procedure3_spec > quxlang::vm_procedure3_impl(instanciation_reference input)
{
    assert(!type_is_contextual(input));
    builtin_function_kind const builtin_kind = co_await rpnx::querygraph::request< function_builtin_query >(input.temploid);
    if (builtin_kind == builtin_function_kind::not_builtin)
    {
        machine_target_info machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
        co_vmir_generator2< rpnx::querygraph::coroutine< vm_procedure3_spec > > generator(machine, input);
        co_return co_await generator.co_generate_functanoid(input);
    }
    if (builtin_kind == builtin_function_kind::builtin_intrinsic)
    {
        throw compiler_bug("builtin intrinsic has no generated VM procedure: " + to_string(input));
    }
    co_return co_await rpnx::querygraph::request< builtin_vm_procedure3_query >(input);
}
