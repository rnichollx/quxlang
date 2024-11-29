// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/res/interpret_bool_resolver.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"
#include <quxlang/compiler.hpp>
#include "quxlang/res/interpret_value_resolver.hpp"

#include "quxlang/macros.hpp"


QUX_CO_RESOLVER_IMPL_FUNC_DEF(interpret_bool)
{
    // Get the general interpret value, check if it's a boolean, and return it.
    // if it's not a boolean, throw an error.
    QUX_CO_GETDEP(val, interpret_value, (input));

    auto booltype = parsers::parse_type_symbol("BOOL");

    if (val.type != booltype)
    {
        throw std::logic_error("Expected boolean value");
    }
    assert(val.data.size() == 1);

    QUX_CO_ANSWER(val.data != std::vector< std::byte >{std::byte{0}});
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(interpret_value)
{
    throw rpnx::unimplemented();
    // co_interpreter interp(c);
    // auto result = co_await interp.eval(input);
    // QUX_CO_ANSWER(result);
}