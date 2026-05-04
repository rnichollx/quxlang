// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/interface_defaultable_spec.hpp>

rpnx::querygraph::coroutine< quxlang::interface_defaultable_spec > quxlang::interface_defaultable_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_interface_declaration >(sym))
    {
        co_return false;
    }
    co_return as< ast2_interface_declaration >(sym).defaultable;
}
