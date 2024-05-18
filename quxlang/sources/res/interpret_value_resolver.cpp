//
// Created by Ryan Nicholl on 3/31/24.
//

#include "quxlang/res/interpret_value_resolver.hpp"

#include "quxlang/macros.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(interpret_value)
{
    co_interpreter interp(c);
    auto result = co_await interp.eval(input);
    QUX_CO_ANSWER(result);
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_interpreter, eval, quxlang::interp_value, (quxlang::expr_interp_input input))
{
   throw rpnx::unimplemented();
}

QUX_SUBCO_MEMBER_FUNC_DEF(co_interpreter::co_expr_interface, create_temporary_storage, quxlang::co_interpreter::co_expr_interface::storage_index, (vm_type type))
{
   throw rpnx::unimplemented();
}