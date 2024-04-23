//
// Created by Ryan Nicholl on 4/22/24.
//
#include <quxlang/res/functum_instanciation.hpp>

#include <quxlang/compiler.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_instanciation)
{
  auto selection = co_await QUX_CO_DEP(function_selection, (input_val));

  co_return co_await QUX_CO_DEP(function_instanciation, (instanciation_reference{.callee = selection, .parameters = input_val.parameters}));
}