// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_eval_string_spec.hpp>

#include <stdexcept>

/// Forces constexpr v3 evaluation to produce a STRING_CONSTANT-compatible result and returns its byte payload.
rpnx::querygraph::coroutine< quxlang::constexpr_eval_string_spec > quxlang::constexpr_eval_string_impl(constexpr_input_v3 input)
{
    input.expected_result_type = readonly_constant{.kind = constant_kind::string};
    auto result = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(std::move(input));
    auto result_it = result.values.find(constexpr_primary_result_id);
    if (result_it == result.values.end() || !typeis< constexpr_string >(result_it->second))
    {
        throw std::logic_error("constexpr string evaluation did not produce a string result");
    }
    co_return as< constexpr_string >(std::move(result_it->second));
}
