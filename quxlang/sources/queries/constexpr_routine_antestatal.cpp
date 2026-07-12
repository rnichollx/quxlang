// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_v3_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include <quxlang/co_vmir_generator2.hpp>
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

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
        for (auto& [name, def] : input.scoped_definitions)
        {
            if (def.template type_is< type_symbol >())
            {
                result.scoped_definitions[std::move(name)] = scoped_typedef{.type = std::move(def.template get_as< type_symbol >())};
                continue;
            }
            throw rpnx::unimplemented();
        }
        for (auto& [name, symbol] : input.scoped_static_symbols)
        {
            result.scoped_definitions[std::move(name)] = scoped_static{.symbol = std::move(symbol)};
        }
        result.statics = std::move(input.static_inputs);
        if (!input.emit_static_results)
        {
            for (auto& [_, binding] : result.statics)
            {
                binding.mutation_result_id.reset();
            }
        }
        return result;
    }
}

rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > quxlang::constexpr_routine_antestatal_impl(constexpr_input2 input)
{
    auto v3_input = make_v3_input(std::move(input));
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > > emitter(machine_info, v3_input.context);
    emitter.set_scoped_definitions_v3(std::move(v3_input.scoped_definitions));
    emitter.set_static_eval_context_v3(std::move(v3_input.statics));
    auto result = co_await emitter.co_generate_constexpr_eval_v3(v3_input.expr, v3_input.expected_result_type);

    co_return std::move(result.routine);
}

/// Generates a constexpr v3 routine and primary AUTO deduction metadata.
rpnx::querygraph::coroutine< quxlang::constexpr_routine_v3_spec > quxlang::constexpr_routine_v3_impl(constexpr_input_v3 input)
{
    std::map< static_local_ref, dependencies > static_dependencies;
    for (std::pair< static_local_ref const, constexpr_static > const& static_input : input.statics)
    {
        dependencies dependency_inventory;
        if (typeis< antestatal_value >(static_input.second.value))
        {
            dependency_inventory = detail::scan_constexpr_static_dependencies(
                constexpr_value_as_antestatal(static_input.second.value), static_input.second.type);
        }
        static_dependencies.emplace(static_input.first, std::move(dependency_inventory));
    }

    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_v3_spec > > emitter(machine_info, input.context);
    emitter.set_scoped_definitions_v3(std::move(input.scoped_definitions));
    emitter.set_static_eval_context_v3(std::move(input.statics));
    constexpr_routine_v3_result result = co_await emitter.co_generate_constexpr_eval_v3(input.expr, input.expected_result_type);
    result.direct_dependencies = detail::scan_routine_dependencies(result.routine, dependency_set::constexpr_);
    result.static_dependencies = std::move(static_dependencies);
    co_return result;
}
