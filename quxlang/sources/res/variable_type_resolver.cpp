//
// Created by Ryan Nicholl on 8/24/2024.
//
#include "quxlang/res/variable.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(variable_type)
{
    auto decls = co_await QUX_CO_DEP(symboid_subdeclaroids, (input));

    for (subdeclaroid const & x : decls)
    {}
    // TODO

    rpnx::unimplemented();
    co_return void_type{};
}