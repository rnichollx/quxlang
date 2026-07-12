// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_routine_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include <quxlang/co_vmir_generator2.hpp>
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

#include "vmir_dependency_scanning.hpp"

rpnx::querygraph::coroutine< quxlang::constexpr_routine_spec > quxlang::constexpr_routine_impl(constexpr_input2 input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_spec > > emitter(machine_info, input.context);
    emitter.set_scoped_definitions(input.scoped_definitions);
    constexpr_routine_result result;
    result.routine = co_await emitter.co_generate_constexpr_eval(input.expr, input.type);
    result.direct_dependencies = detail::scan_routine_dependencies(result.routine, dependency_set::constexpr_);
    for (std::pair< static_local_ref const, constexpr_static > const& static_input : input.static_inputs)
    {
        dependencies static_dependencies;
        if (typeis< antestatal_value >(static_input.second.value))
        {
            static_dependencies = detail::scan_constexpr_static_dependencies(
                constexpr_value_as_antestatal(static_input.second.value), static_input.second.type);
        }
        result.static_dependencies.emplace(static_input.first, std::move(static_dependencies));
    }

    co_return result;
}
