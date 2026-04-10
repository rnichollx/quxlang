// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"

rpnx::querygraph::coroutine< quxlang::constexpr_eval_spec > quxlang::constexpr_eval_impl(constexpr_input2 input)
{
    vmir2::ir2_constexpr_interpreter interp;

    auto ir3 = co_await rpnx::querygraph::request< constexpr_routine_query >(input);

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
            vmir2::functanoid_routine3 const& ir2_other = co_await rpnx::querygraph::request< vm_procedure3_query >(funcname.template get_as< instanciation_reference >());

            interp.add_functanoid3(funcname, ir2_other);
        }
    }

    interp.exec3(void_type{});

    auto val = interp.get_cr_value();
    val.type = type_symbol(bool_type{});
    co_return val;
}
