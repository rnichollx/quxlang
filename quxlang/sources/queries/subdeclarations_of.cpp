// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/subdeclarations_of_spec.hpp>

rpnx::querygraph::coroutine< quxlang::subdeclarations_of_spec > quxlang::subdeclarations_of_impl(type_symbol input)
{
    ast2_symboid const& sym = co_await rpnx::querygraph::request< symboid_query >(input);
    co_return rpnx::apply_visitor< std::vector< subdeclaration > >(sym, []< typename Declaration >(Declaration const& declaration) -> std::vector< subdeclaration >
    {
        if constexpr (requires { declaration.declarations; })
        {
            return declaration.declarations;
        }
        else
        {
            return {};
        }
    });
}
