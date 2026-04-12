// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include <quxlang/co_vmir_generator2.hpp>
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > quxlang::constexpr_routine_antestatal_impl(constexpr_input2 input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > > emitter(machine_info, input.context);
    emitter.set_scoped_definitions(input.scoped_definitions);
    auto result = co_await emitter.co_generate_constexpr_eval_antestatal(input.expr, input.type);

    co_return result;
}
