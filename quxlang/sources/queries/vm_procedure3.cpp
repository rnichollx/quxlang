// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/vm_procedure3_spec.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>

#include "quxlang/manipulators/typeutils.hpp"




rpnx::querygraph::coroutine< quxlang::vm_procedure3_spec > quxlang::vm_procedure3_impl(instanciation_reference input)
{
    assert(!type_is_contextual(input));
    builtin_function_kind const builtin_kind = co_await rpnx::querygraph::request< function_builtin_query >(input.temploid);
    if (builtin_kind == builtin_function_kind::not_builtin)
    {
        co_return co_await rpnx::querygraph::request< user_vm_procedure3_query >(input);
    }
    if (builtin_kind == builtin_function_kind::builtin_intrinsic)
    {
        throw compiler_bug("builtin intrinsic has no generated VM procedure: " + to_string(input));
    }
    co_return co_await rpnx::querygraph::request< builtin_vm_procedure3_query >(input);
}
