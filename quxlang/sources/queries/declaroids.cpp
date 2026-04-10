// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/declaroids_spec.hpp>


rpnx::querygraph::coroutine< quxlang::declaroids_spec > quxlang::declaroids_impl(type_symbol input)
{
    std::vector< declaroid > output;

    std::string inputname = to_string(input);

    if (typeis< absolute_module_reference >(input))
    {
        throw std::logic_error("Cannot have declarations of a module");
    }

    if (typeis< initialization_reference >(input))
    {
        throw std::logic_error("Non-canonical symbol passed to declaroids resolver: initialization_reference. Canonicalize with lookup/instanciation before calling declaroids.");
    }

    bool is_member = false;
    std::string subname;

    if (typeis< subsymbol >(input))
    {
        subname = as< subsymbol >(input).name;
    }
    else if (typeis< submember >(input))
    {
        is_member = true;
        subname = as< submember >(input).name;
    }
    else
    {
        co_return {};
    }

    std::optional< type_symbol > parent_addr = type_parent(input);

    if (!parent_addr)
    {
        co_return {};
    }

    std::vector< subdeclaroid > subdeclaroids = co_await rpnx::querygraph::request< symboid_subdeclaroids_query >(parent_addr.value());

    for (auto& subdecl : subdeclaroids)
    {
        if (typeis< member_subdeclaroid >(subdecl) && is_member && subname == as< member_subdeclaroid >(subdecl).name)
        {
            auto const& member = as< member_subdeclaroid >(subdecl);

            bool included = true;
            if (member.include_if)
            {
                constexpr_input cx_input;
                cx_input.context = parent_addr.value();
                cx_input.expr = *member.include_if;
                included = co_await rpnx::querygraph::request< constexpr_bool_query >(cx_input);
            }

            if (!included)
            {
                continue;
            }
            output.push_back(member.decl);
        }
        else if (typeis< global_subdeclaroid >(subdecl) && !is_member && subname == as< global_subdeclaroid >(subdecl).name)
        {
            auto const& global = as< global_subdeclaroid >(subdecl);
            bool included = true;
            if (global.include_if)
            {
                constexpr_input cx_input;
                cx_input.context = parent_addr.value();
                cx_input.expr = *global.include_if;
                included = co_await rpnx::querygraph::request< constexpr_bool_query >(cx_input);
            }
            if (!included)
            {
                continue;
            }
            output.push_back(as< global_subdeclaroid >(subdecl).decl);
        }
    }

    co_return output;
}
