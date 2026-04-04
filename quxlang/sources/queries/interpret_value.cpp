// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/interpret_value_spec.hpp>
#include "quxlang/parsers/parse_type_symbol.hpp"

#include "quxlang/macros.hpp"



rpnx::querygraph::coroutine< quxlang::interpret_value_spec > quxlang::interpret_value_impl(expr_interp_input input)
{
    throw rpnx::unimplemented();
    // co_interpreter interp(c);
    // auto result = co_await interp.eval(input);
    // co_return result;
}