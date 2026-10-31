// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/specs/builtin_datatype_compare_vm_procedure3_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>

rpnx::querygraph::coroutine< quxlang::builtin_datatype_compare_vm_procedure3_spec > quxlang::builtin_datatype_compare_vm_procedure3_impl(instanciation_reference input)
{
    machine_target_info const& machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::builtin_datatype_compare_vm_procedure3_spec > > gen(machine_info, input);

    type_symbol const& compared_type = input.temploid.templexoid.get_as< submember >().of;
    if (co_await rpnx::querygraph::request< symbol_type_query >(compared_type) == symbol_kind::class_)
    {
        class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(compared_type);
        if (kind == class_kind::variant || kind == class_kind::union_)
        {
            co_return co_await gen.co_generate_builtin_fusion_compare(input);
        }
    }
    co_return co_await gen.co_generate_builtin_datatype_compare(input);
}
