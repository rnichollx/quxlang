// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/vm_procedure3_spec.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>





rpnx::querygraph::coroutine< quxlang::vm_procedure3_spec > quxlang::vm_procedure3_impl(instanciation_reference input)
{
    assert(!type_is_contextual(input));
    if (co_await rpnx::querygraph::query_request< function_builtin_query >(input.temploid))
    {
        co_return co_await rpnx::querygraph::query_request< builtin_vm_procedure3_query >(input);
    }
    else
    {
        co_return co_await rpnx::querygraph::query_request< user_vm_procedure3_query >(input);
    }
}
