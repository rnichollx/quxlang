// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_vm_procedure3_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include <quxlang/co_vmir_generator2.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>





rpnx::querygraph::coroutine< quxlang::user_vm_procedure3_spec > quxlang::user_vm_procedure3_impl(instanciation_reference input)
{
    auto const machine_info = co_await rpnx::querygraph::query_request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::user_vm_procedure3_spec > > gen(machine_info, input);

    co_return co_await gen.co_generate_functanoid(input);
}
