// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_v3_spec.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include <quxlang/co_vmir_generator2.hpp>

#include "vmir_dependency_scanning.hpp"

namespace quxlang
{
    /// Converts legacy constexpr routine input into constexpr v3 routine input.
    auto make_v3_input(constexpr_input2 input) -> constexpr_input_v3
    {
        constexpr_input_v3 result;
        result.expr = std::move(input.expr);
        result.context = std::move(input.context);
        result.expected_result_type = input.require_antestatal_result ? std::optional< type_symbol >(std::move(input.type)) : std::nullopt;
        result.antestatal_global_symbol = std::move(input.antestatal_global_symbol);
        result.mutable_statics = input.emit_static_results;
        return result;
    }
} // namespace quxlang

rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > quxlang::constexpr_routine_antestatal_impl(constexpr_input2 input)
{
    auto v3_input = make_v3_input(std::move(input));
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > > emitter(machine_info, v3_input.context);
    emitter.set_static_evaluation(v3_input.mutable_statics);
    auto result = co_await emitter.co_generate_constexpr_eval_v3(v3_input.expr, v3_input.expected_result_type);

    co_return std::move(result.routine);
}

/// Generates a constexpr v3 routine and primary AUTO deduction metadata.
rpnx::querygraph::coroutine< quxlang::constexpr_routine_v3_spec > quxlang::constexpr_routine_v3_impl(constexpr_input_v3 input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_v3_spec > > emitter(machine_info, input.context);
    emitter.set_static_evaluation(input.mutable_statics);
    constexpr_routine_v3_result result = co_await emitter.co_generate_constexpr_eval_v3(input.expr, input.expected_result_type);
    result.direct_dependencies = detail::scan_routine_dependencies(result.routine, dependency_set::constexpr_);
    for (static_local_ref const& symbol : emitter.materialized_static_symbols())
    {
        constexpr_static object = co_await co_find_body_static< rpnx::querygraph::coroutine< constexpr_routine_v3_spec > >(input.context, symbol);
        result.static_dependencies.emplace(symbol, detail::scan_constexpr_static_dependencies(constexpr_value_as_antestatal(object.value), object.type));
    }
    co_return result;
}
