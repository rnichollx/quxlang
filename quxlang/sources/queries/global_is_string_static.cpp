// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/global_is_string_static_spec.hpp>

/// Checks that a symbol is specifically a global variable declared STATIC with STRING_CONSTANT type.
rpnx::querygraph::coroutine< quxlang::global_is_string_static_spec > quxlang::global_is_string_static_impl(type_symbol input)
{
    auto kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    if (kind != symbol_kind::global_variable)
    {
        co_return false;
    }

    auto symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        co_return false;
    }

    auto const& decl = as< ast2_variable_declaration >(symboid);
    if (!decl.keyword_tags.contains("STATIC"))
    {
        co_return false;
    }

    auto variable_type = co_await rpnx::querygraph::request< variable_type_query >(input);
    if (!typeis< readonly_constant >(variable_type))
    {
        co_return false;
    }

    co_return as< readonly_constant >(variable_type).kind == constant_kind::string;
}
