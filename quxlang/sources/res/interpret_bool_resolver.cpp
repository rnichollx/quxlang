//
// Created by Ryan Nicholl on 3/31/24.
//

#include "quxlang/res/interpret_bool_resolver.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"
#include <quxlang/compiler.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(interpret_bool)
{
    // Get the general interpret value, check if it's a boolean, and return it.
    // if it's not a boolean, throw an error.
    QUX_CO_GETDEP(val, interpret_value, (input));

    auto booltype = parsers::parse_type_symbol("BOOL");

    if (val.type != booltype)
    {
        throw std::runtime_error("Expected boolean value");
    }
    assert(val.data.size() == 1);

    QUX_CO_ANSWER(val.data != std::vector< std::byte >{std::byte{0}});
}