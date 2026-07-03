// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/constexpr_eval_numeric_spec.hpp>

#include <stdexcept>

rpnx::querygraph::coroutine< quxlang::constexpr_eval_numeric_spec > quxlang::constexpr_eval_numeric_impl(constexpr_input_v3 input)
{
    input.expected_result_type = readonly_constant{.kind = constant_kind::numeric};
    auto result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(std::move(input));
    auto result_it = result.values.find(constexpr_primary_result_id);
    if (result_it == result.values.end() || !typeis< constexpr_numeric >(result_it->second))
    {
        throw quxlang::semantic_compilation_error("constexpr numeric evaluation did not produce a numeric result");
    }
    co_return as< constexpr_numeric >(std::move(result_it->second));
}
