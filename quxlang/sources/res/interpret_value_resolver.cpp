// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/res/interpret_value_resolver.hpp"

#include "quxlang/macros.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(interpret_value)
{
    throw rpnx::unimplemented();
    // co_interpreter interp(c);
    // auto result = co_await interp.eval(input);
    // QUX_CO_ANSWER(result);
}
