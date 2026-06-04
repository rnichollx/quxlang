// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/queries/specs/global_init_type_spec.hpp>

rpnx::querygraph::coroutine< quxlang::global_init_type_spec > quxlang::global_init_type_impl(type_symbol input)
{
    if ((co_await rpnx::querygraph::request< symbol_type_query >(input)) != symbol_kind::global_variable)
    {
        co_return initialization_type::init_with_guard;
    }

    ast2_symboid symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        co_return initialization_type::init_with_guard;
    }

    ast2_variable_declaration const& decl = as< ast2_variable_declaration >(symboid);
    if (decl.init_expr.has_value() || !decl.init_args.empty())
    {
        co_return initialization_type::init_with_guard;
    }

    type_symbol const variable_type = co_await rpnx::querygraph::request< variable_type_query >(input);
    if (co_await rpnx::querygraph::request< type_is_trivially_default_constructible_query >(variable_type))
    {
        co_return initialization_type::init_trivial;
    }

    co_return initialization_type::init_with_guard;
}
