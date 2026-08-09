// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/active_symboid_subdeclaroids_spec.hpp>

#include <optional>
#include <utility>

rpnx::querygraph::coroutine< quxlang::active_symboid_subdeclaroids_spec > quxlang::active_symboid_subdeclaroids_impl(type_symbol input)
{
    std::vector< subdeclaroid > const& declarations = co_await rpnx::querygraph::request< symboid_subdeclaroids_query >(input);
    std::vector< subdeclaroid > output;
    output.reserve(declarations.size());

    for (subdeclaroid const& declaration : declarations)
    {
        std::optional< expression > const* include_if;
        if (declaration.type_is< member_subdeclaroid >())
        {
            include_if = &declaration.get_as< member_subdeclaroid >().include_if;
        }
        else
        {
            include_if = &declaration.get_as< global_subdeclaroid >().include_if;
        }
        include_if_is_active_input condition{
            .context = input,
            .condition = *include_if,
        };
        if (co_await rpnx::querygraph::request< include_if_is_active_query >(std::move(condition)))
        {
            output.push_back(declaration);
        }
    }

    co_return output;
}
