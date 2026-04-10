// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/interpret_bool_spec.hpp>
#include "quxlang/parsers/parse_type_symbol.hpp"

#include "quxlang/macros.hpp"



rpnx::querygraph::coroutine< quxlang::interpret_bool_spec > quxlang::interpret_bool_impl(expr_interp_input input)
{
    // Get the general interpret value, check if it's a boolean, and return it.
    // if it's not a boolean, throw an error.
    auto val = co_await rpnx::querygraph::request< interpret_value_query >(input);

    auto booltype = parsers::parse_type_symbol("BOOL");

    if (val.type != booltype)
    {
        throw std::logic_error("Expected boolean value");
    }
    assert(val.data.size() == 1);

    co_return val.data != std::vector< std::byte >{std::byte{0}};
}
