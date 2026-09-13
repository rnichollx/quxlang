// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/specs/constexpr_routine_spec.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include <quxlang/co_vmir_generator2.hpp>

#include "vmir_dependency_scanning.hpp"

rpnx::querygraph::coroutine< quxlang::constexpr_routine_spec > quxlang::constexpr_routine_impl(constexpr_input2 input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_spec > > emitter(machine_info, input.context);

    constexpr_routine_result result;
    result.routine = co_await emitter.co_generate_constexpr_eval(input.expr, input.type);
    result.direct_dependencies = detail::scan_routine_dependencies(result.routine, dependency_set::constexpr_);

    co_return result;
}
