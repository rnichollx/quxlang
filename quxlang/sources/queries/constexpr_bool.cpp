// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_bool_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "query_helpers.hpp"

namespace quxlang::detail
{
    struct constexpr_bool_helpers
    {
        /// Converts legacy constexpr bool input into the constexpr v3 input shape.
        static auto make_v3_input(constexpr_input input) -> constexpr_input_v3
        {
        constexpr_input_v3 result;
        result.context = std::move(input.context);
        result.expr = std::move(input.expr);
        result.expected_result_type = quxlang::bool_type{};
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

rpnx::querygraph::coroutine< quxlang::constexpr_bool_spec > quxlang::constexpr_bool_impl(constexpr_input input)
{
    auto eval = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(detail::constexpr_bool_helpers::make_v3_input(std::move(input)));
    auto result_it = eval.values.find(constexpr_primary_result_id);
    if (result_it == eval.values.end())
    {
        throw compiler_bug("constexpr_bool did not produce result id 0");
    }
    auto const& value = constexpr_value_as_antestatal(result_it->second);
    if (!typeis< antestatal_primitive >(value))
    {
        throw compiler_bug("constexpr_bool result is not primitive");
    }
    auto const& bytes = as< antestatal_primitive >(value).value;

    if (bytes == std::vector{std::byte{0}})
    {

        co_return false;
    }
    else if (bytes == std::vector{std::byte{1}})
    {
        co_return true;
    }
    else

    {
        throw compiler_bug("shouldnt get here");
    }

}
