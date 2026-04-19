// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp>
#include <quxlang/queries/specs/constexpr_routine_v3_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include <quxlang/co_vmir_generator2.hpp>
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

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

    template < typename Spec >
    auto apply_context_instantiation_scopes(type_symbol context, constexpr_input_v3& input) -> typename rpnx::querygraph::coroutine< Spec >::template cosubroutine< void >
    {
        std::optional< type_symbol > current_context = std::move(context);
        while (current_context.has_value())
        {
            if (typeis< instanciation_reference >(*current_context))
            {
                auto const& inst = as< instanciation_reference >(*current_context);
                auto inst_kind = co_await rpnx::querygraph::request< symbol_type_query >(inst.temploid);
                if (inst_kind == symbol_kind::template_)
                {
                    merge_instantiation_scope_bindings(instantiation_scope_for(inst), input);
                }
            }
            current_context = type_parent(*current_context);
        }
        co_return;
    }
}

rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > quxlang::constexpr_routine_antestatal_impl(constexpr_input2 input)
{
    auto v3_input = make_v3_input(std::move(input));
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_antestatal_spec > > emitter(machine_info, v3_input.context);
    co_await apply_context_instantiation_scopes< constexpr_routine_antestatal_spec >(v3_input.context, v3_input);
    emitter.set_scoped_definitions_v3(std::move(v3_input.scoped_definitions));
    emitter.set_static_eval_context_v3(std::move(v3_input.statics));
    auto result = co_await emitter.co_generate_constexpr_eval_v3(v3_input.expr, v3_input.expected_result_type);

    co_return std::move(result.routine);
}

/// Generates a constexpr v3 routine and primary AUTO deduction metadata.
rpnx::querygraph::coroutine< quxlang::constexpr_routine_v3_spec > quxlang::constexpr_routine_v3_impl(constexpr_input_v3 input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});
    co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::constexpr_routine_v3_spec > > emitter(machine_info, input.context);
    co_await apply_context_instantiation_scopes< constexpr_routine_v3_spec >(input.context, input);
    emitter.set_scoped_definitions_v3(std::move(input.scoped_definitions));
    emitter.set_static_eval_context_v3(std::move(input.statics));
    co_return co_await emitter.co_generate_constexpr_eval_v3(input.expr, input.expected_result_type);
}
