// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/vm_procedure3_spec.hpp>

#include <quxlang/co_vmir_generator2.hpp>
#include <quxlang/co_coroutine_initialize.hpp>
#include <quxlang/co_coroutine_cleanup.hpp>
#include <quxlang/vmir2/coroutine_lowering.hpp>

#include "quxlang/manipulators/typeutils.hpp"




rpnx::querygraph::coroutine< quxlang::vm_procedure3_spec > quxlang::vm_procedure3_impl(instanciation_reference input)
{
    assert(!type_is_contextual(input));
    if (input.temploid.templexoid.type_is< subsymbol >())
    {
        subsymbol const& symbol = input.temploid.templexoid.get_as< subsymbol >();
        if (symbol.name == "__COROUTINE_DESTRUCTOR" || symbol.name == "__COROUTINE_SUBOBJECT_DESTRUCTOR")
        {
            machine_target_info const& machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
            using coroutine_base = rpnx::querygraph::coroutine< vm_procedure3_spec >;
            co_vmir_generator2< coroutine_base > generator(machine, input);
            coroutine_cleanup_lowering< coroutine_base > cleanup(generator);
            co_return co_await cleanup.generate_destructor(input, symbol.of, symbol.name == "__COROUTINE_SUBOBJECT_DESTRUCTOR");
        }
    }
    if (std::optional< coroutine_frame_operation > operation = coroutine_frame_operation_symbol(input.temploid.templexoid))
    {
        if (operation->name == "RESUME")
        {
            coroutine_frame_info const& frame = co_await rpnx::querygraph::request< coroutine_frame_query >(operation->coroutine);
            vmir2::functanoid_routine3 const& logical = co_await rpnx::querygraph::request< vm_procedure3_query >(operation->coroutine);
            co_return vmir2::lower_coroutine_resume(logical, frame);
        }
        if (operation->name == "TRY_CANCEL")
        {
            coroutine_frame_info const& frame = co_await rpnx::querygraph::request< coroutine_frame_query >(operation->coroutine);
            vmir2::functanoid_routine3 const& logical = co_await rpnx::querygraph::request< vm_procedure3_query >(operation->coroutine);
            co_return vmir2::lower_coroutine_try_cancel(logical, frame);
        }
        if (operation->name == "INITIALIZE")
        {
            machine_target_info machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
            coroutine_frame_info const& frame = co_await rpnx::querygraph::request< coroutine_frame_query >(operation->coroutine);
            co_vmir_generator2< rpnx::querygraph::coroutine< vm_procedure3_spec > > generator(machine, input);
            co_return co_await co_generate_coroutine_initialize(generator, input, operation->coroutine, frame);
        }
    }
    if (input.temploid.templexoid.type_is< submember >() && input.temploid.templexoid.get_as< submember >().name == "DESTRUCTOR")
    {
        std::optional< ast2_function_declaration > const& declaration = co_await rpnx::querygraph::request< function_declaration_query >(input.temploid);
        if (declaration.has_value() && declaration->header.execution_kind == function_execution_kind::coroutine)
        {
            machine_target_info const& machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
            using coroutine_base = rpnx::querygraph::coroutine< vm_procedure3_spec >;
            co_vmir_generator2< coroutine_base > generator(machine, input);
            coroutine_cleanup_lowering< coroutine_base > cleanup(generator);
            co_return co_await cleanup.generate_destructor(input, input.temploid.templexoid.get_as< submember >().of, false, input);
        }
    }
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
