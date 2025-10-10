// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler.hpp>
#include <quxlang/res/declaroids_resolver.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(declaroids)
{

    std::vector< declaroid > output;

    std::string inputname = to_string(input);

    if (typeis< absolute_module_reference >(input))
    {
        throw std::logic_error("Cannot have declarations of a module");
    }

    if (typeis< initialization_reference >(input))
    {
        // TODO: Maybe we allow this for templates?
        throw std::logic_error("Instancations are not declarables");
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

    std::vector< subdeclaroid > subdeclaroids = co_await QUX_CO_DEP(symboid_subdeclaroids, (parent_addr.value()));

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
                included = co_await QUX_CO_DEP(constexpr_bool, (cx_input));
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
                included = co_await QUX_CO_DEP(constexpr_bool, (cx_input));
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