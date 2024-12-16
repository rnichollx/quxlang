#include "quxlang/res/constexpr.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_interpreter.hpp"
//
// Created by Ryan Nicholl on 12/14/2024.
//
QUX_CO_RESOLVER_IMPL_FUNC_DEF(constexpr_bool)
{
    expr_ir2_input inp;
    inp.context = input_val.context;
    inp.expr = input_val.expr;

    auto ir2 = co_await QUX_CO_DEP(expr_ir2, (inp));

    vmir2::ir2_interpreter interp;

    interp.add_functanoid(void_type{}, ir2);

    interp.exec(void_type{});

    co_return interp.get_cr_bool();
}