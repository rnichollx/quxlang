// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/active_subdeclaroids_spec.hpp>

rpnx::querygraph::coroutine< quxlang::active_subdeclaroids_spec > quxlang::active_subdeclaroids_impl(type_symbol input)
{
    type_symbol parent;
    std::string name;
    bool is_member = false;

    if (input.type_is< subsymbol >())
    {
        subsymbol const& symbol = input.get_as< subsymbol >();
        parent = symbol.of;
        name = symbol.name;
    }
    else if (input.type_is< submember >())
    {
        submember const& member = input.get_as< submember >();
        parent = member.of;
        name = member.name;
        is_member = true;
    }
    else
    {
        co_return {};
    }

    std::vector< subdeclaroid > output;
    std::vector< subdeclaroid > const& declarations = co_await rpnx::querygraph::request< symboid_subdeclaroids_query >(parent);

    for (subdeclaroid const& declaration : declarations)
    {
        std::optional< expression > include_if;
        if (is_member && declaration.type_is< member_subdeclaroid >())
        {
            member_subdeclaroid const& member = declaration.get_as< member_subdeclaroid >();
            if (member.name != name)
            {
                continue;
            }
            include_if = member.include_if;
        }
        else if (!is_member && declaration.type_is< global_subdeclaroid >())
        {
            global_subdeclaroid const& global = declaration.get_as< global_subdeclaroid >();
            if (global.name != name)
            {
                continue;
            }
            include_if = global.include_if;
        }
        else
        {
            continue;
        }

        if (include_if.has_value())
        {
            constexpr_input input_condition{
                .context = parent,
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
