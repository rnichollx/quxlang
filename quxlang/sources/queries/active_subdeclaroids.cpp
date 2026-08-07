// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/active_subdeclaroids_spec.hpp>

rpnx::querygraph::coroutine< quxlang::active_subdeclaroids_spec > quxlang::active_subdeclaroids_impl(type_symbol input)
{
    std::vector< subdeclaroid > output;
    std::vector< subdeclaroid > const& declarations = co_await rpnx::querygraph::request< symboid_subdeclaroids_query >(input);

    for (subdeclaroid const& declaration : declarations)
    {
        std::optional< expression > include_if;
        if (declaration.type_is< member_subdeclaroid >())
        {
            include_if = declaration.get_as< member_subdeclaroid >().include_if;
        }
        else
        {
            include_if = declaration.get_as< global_subdeclaroid >().include_if;
        }

        if (include_if.has_value())
        {
            constexpr_input input_condition{
                .context = input,
                .expr = *include_if,
            };
            if (!(co_await rpnx::querygraph::request< constexpr_bool_query >(std::move(input_condition))))
            {
                continue;
            }
        }
        output.push_back(declaration);
    }

    co_return output;
}
