// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_u64_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

rpnx::querygraph::coroutine< quxlang::constexpr_u64_spec > quxlang::constexpr_u64_impl(constexpr_input input)
{
    constexpr_input2 inp;
    inp.context = input.context;
    inp.expr = input.expr;
    inp.type = int_type{.bits = 64, .has_sign = false}; // We want a boolean result

    auto eval = co_await rpnx::querygraph::query_request< constexpr_eval_query >(inp);

    auto data = eval.value.get();

    auto [intval, ok] = bytemath::le_to_u< std::uint64_t >(data);
    if (!ok)
    {
        throw compiler_bug("Error in constexpr_u64: result is not a valid u64");
    }
    co_return intval;
}
