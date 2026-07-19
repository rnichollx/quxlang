// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_u64_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "query_helpers.hpp"

namespace quxlang::detail
{
    struct constexpr_u64_helpers
    {
        /// Converts legacy constexpr u64 input into the constexpr v3 input shape.
        static auto make_v3_input(constexpr_input input) -> constexpr_input_v3
        {
        constexpr_input_v3 result;
        result.context = std::move(input.context);
        result.expr = std::move(input.expr);
        result.expected_result_type = quxlang::int_type{.bits = 64, .has_sign = false};
        for (auto& [name, def] : input.scoped_definitions)
        {
            if (def.template type_is< quxlang::type_symbol >())
            {
                result.scoped_definitions[std::move(name)] = quxlang::scoped_typedef{.type = std::move(def.template get_as< quxlang::type_symbol >())};
                continue;
            }
            throw rpnx::unimplemented();
        }
        for (auto& [name, symbol] : input.scoped_static_symbols)
        {
            result.scoped_definitions[std::move(name)] = quxlang::scoped_static{.symbol = std::move(symbol)};
        }
        result.statics = std::move(input.static_inputs);
        for (auto& [_, binding] : result.statics)
        {
            binding.mutation_result_id.reset();
        }
            return result;
        }
    };
} // namespace quxlang::detail

rpnx::querygraph::coroutine< quxlang::constexpr_u64_spec > quxlang::constexpr_u64_impl(constexpr_input input)
{
    auto eval = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(detail::constexpr_u64_helpers::make_v3_input(std::move(input)));
    auto result_it = eval.values.find(constexpr_primary_result_id);
    if (result_it == eval.values.end())
    {
        throw compiler_bug("constexpr_u64 did not produce result id 0");
    }
    auto const& value = constexpr_value_as_antestatal(result_it->second);
    if (!typeis< antestatal_primitive >(value))
    {
        throw compiler_bug("constexpr_u64 result is not primitive");
    }
    auto const& data = as< antestatal_primitive >(value).value;

    auto [intval, ok] = bytemath::le_to_u< std::uint64_t >(data);
    if (!ok)
    {
        throw compiler_bug("Error in constexpr_u64: result is not a valid u64");
    }
    co_return intval;
}
