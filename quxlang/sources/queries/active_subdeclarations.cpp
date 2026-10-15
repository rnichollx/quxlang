// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/active_subdeclarations_spec.hpp>

#include <optional>
#include <utility>

rpnx::querygraph::coroutine< quxlang::active_subdeclarations_spec > quxlang::active_subdeclarations_impl(type_symbol input)
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

    std::vector< subdeclaration > output;
    std::vector< subdeclaration > const& declarations = co_await rpnx::querygraph::request< subdeclarations_of_query >(parent);

    for (subdeclaration const& declaration : declarations)
    {
        std::optional< expression > const* include_if;
        if (is_member && declaration.type_is< member_subdeclaration >())
        {
            member_subdeclaration const& member = declaration.get_as< member_subdeclaration >();
            if (member.name != name)
            {
                continue;
            }
            include_if = &member.include_if;
        }
        else if (!is_member && declaration.type_is< global_subdeclaration >())
        {
            global_subdeclaration const& global = declaration.get_as< global_subdeclaration >();
            if (global.name != name)
            {
                continue;
            }
            include_if = &global.include_if;
        }
        else
        {
            continue;
        }

        include_if_is_active_input condition{
            .context = parent,
            .condition = *include_if,
        };
        if (co_await rpnx::querygraph::request< include_if_is_active_query >(std::move(condition)))
        {
            output.push_back(declaration);
        }
    }

    co_return output;
}
