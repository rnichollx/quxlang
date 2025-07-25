#include "quxlang/res/constexpr.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/compiler_binding.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/res/expr/co_vmir_codegen_emitter.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
//
// Created by Ryan Nicholl on 12/14/2024.
//
QUX_CO_RESOLVER_IMPL_FUNC_DEF(constexpr_bool)
{
    constexpr_input2 inp;
    inp.context = input_val.context;
    inp.expr = input_val.expr;
    inp.type = bool_type{}; // We want a boolean result

    auto eval = co_await QUX_CO_DEP(constexpr_eval, (inp));

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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(constexpr_u64)
{
    constexpr_input2 inp;
    inp.context = input_val.context;
    inp.expr = input_val.expr;
    inp.type = bool_type{}; // We want a boolean result

    auto eval = co_await QUX_CO_DEP(constexpr_eval, (inp));

    co_return interp.get_cr_u64();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(constexpr_routine)
{
    quxlang::vmir2::functanoid_routine3 r;

    std::vector< bool > temp;
    quxlang::compiler_binder binder(c);
    co_vmir_generator< quxlang::compiler_binder > emitter(binder, input.context);
    auto result = co_await emitter.co_generate_constexpr_eval(input.expr, input.type);

    co_return result;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(constexpr_eval)
{
    vmir2::ir2_constexpr_interpreter interp;

    auto ir3 = co_await QUX_CO_DEP(constexpr_routine, (constexpr_input2{input.expr, input.context, input.type}));

    constexpr_input2 inp;
    inp.context = input_val.context;
    inp.expr = input_val.expr;
    inp.type = bool_type{}; // We want a boolean result

    interp.add_functanoid3(void_type{}, ir3);

    while (!interp.missing_functanoids().empty())
    {
        auto missing_functanoids = interp.missing_functanoids();

        for (type_symbol const& funcname : missing_functanoids)
        {
            if (!typeis< instanciation_reference >(funcname))
            {
                throw compiler_bug("Internal Compiler Error: Missing functanoid is not an instanciation reference");
            }
            vmir2::functanoid_routine3 const& ir2_other = co_await QUX_CO_DEP(vm_procedure3, (funcname.template get_as< instanciation_reference >()));

            interp.add_functanoid3(funcname, ir2_other);
        }
    }

    interp.exec3(void_type{});

    auto val = interp.get_cr_value();
    val.type = type_symbol( bool_type{});
    co_return val;

}