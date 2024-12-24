// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/res/function.hpp>
#include <quxlang/compiler.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_builtin)
{
    auto builtin_overloads = co_await QUX_CO_DEP(list_builtin_functum_overloads, (input_val.templexoid));

    for (auto const& info : builtin_overloads)
    {
        if (info.overload == input_val.overload)
        {
            QUX_CO_ANSWER(info);
        }
    }

    co_return std::nullopt;
}