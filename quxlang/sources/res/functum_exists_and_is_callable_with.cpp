//
// Created by Ryan Nicholl on 4/22/24.
//
#include "quxlang/macros.hpp"
#include <quxlang/res/functum_exists_and_is_callable_with_resolver.hpp>

#include <quxlang/compiler.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_exists_and_is_callable_with)
{
    auto ol = co_await QUX_CO_DEP(functum_instanciation, (input_val));

    QUX_CO_ANSWER(ol.has_value());
}