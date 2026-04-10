// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_bool_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

rpnx::querygraph::coroutine< quxlang::constexpr_bool_spec > quxlang::constexpr_bool_impl(constexpr_input input)
{
    constexpr_input2 inp;
    inp.context = input.context;
    inp.expr = input.expr;
    inp.type = bool_type{}; // We want a boolean result
    inp.scoped_definitions = input.scoped_definitions;

    auto eval = co_await rpnx::querygraph::request< constexpr_eval_query >(inp);

    if (eval.value == std::vector{std::byte{0}})
    {

        co_return false;
    }
    else if (eval.value == std::vector{std::byte{1}})
    {
        co_return true;
    }
    else

    {
        throw compiler_bug("shouldnt get here");
    }

}
