//
// Created by Ryan Nicholl on 4/26/24.
//

#include <quxlang/res/function_builtin.hpp>
#include <quxlang/compiler.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_builtin)
{
    auto builtin_overloads = co_await QUX_CO_DEP(list_builtin_functum_overloads, (input_val.callee));

    for (auto const& info : builtin_overloads)
    {
        if (info.overload == input_val.overload)
        {
            QUX_CO_ANSWER(info);
        }
    }

    co_return std::nullopt;
}