// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/res/functum_instanciation.hpp>

#include <quxlang/compiler.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_instanciation)
{
    auto selection = co_await QUX_CO_DEP(functum_select_function, (input_val));

    if (!selection)
    {
        QUX_WHY("No function found that matches the given parameters.");

        QUX_CO_ANSWER(std::nullopt);
        // throw std::logic_error("No function found that matches the given parameters.");
    }

    co_return co_await QUX_CO_DEP(function_instanciation, (instanciation_reference{.callee = selection.value(), .parameters = input_val.parameters}));
}