// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/builtin_default_ctor_vm_procedure3_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include <quxlang/co_vmir_generator2.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>





rpnx::querygraph::coroutine< quxlang::builtin_default_ctor_vm_procedure3_spec > quxlang::builtin_default_ctor_vm_procedure3_impl(instanciation_reference input)
{
    std::string input_name = quxlang::to_string(input);
    auto const machine_info = co_await rpnx::querygraph::query_request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::builtin_default_ctor_vm_procedure3_spec > > gen(machine_info, input);

    co_return co_await gen.co_generate_builtin_ctor(input);
}
