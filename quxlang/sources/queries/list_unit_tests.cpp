// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/list_unit_tests_spec.hpp>

rpnx::querygraph::coroutine< quxlang::list_unit_tests_spec > quxlang::list_unit_tests_impl(type_symbol input)
{
    std::vector< subdeclaroid > const input_subdeclaroids = co_await rpnx::querygraph::request< symboid_subdeclaroids_query >(input);

    std::set< type_symbol > result;

    for (subdeclaroid const& sdcl : input_subdeclaroids)
    {
        declaroid const* decl = nullptr;
        std::string const* name = nullptr;
        std::optional< expression > const* include_if = nullptr;
        bool is_member = false;

        if (sdcl.type_is< global_subdeclaroid >())
        {
            global_subdeclaroid const& global = sdcl.as< global_subdeclaroid >();
            decl = &global.decl;
            name = &global.name;
            include_if = &global.include_if;
        }
        else if (sdcl.type_is< member_subdeclaroid >())
        {
            member_subdeclaroid const& member = sdcl.as< member_subdeclaroid >();
            decl = &member.decl;
            name = &member.name;
            include_if = &member.include_if;
            is_member = true;
        }
        else
        {
            continue;
        }

        bool included = true;
        if (include_if->has_value())
        {
            constexpr_input input_constexpr;
            input_constexpr.context = input;
            input_constexpr.expr = **include_if;
            included = co_await rpnx::querygraph::request< constexpr_bool_query >(input_constexpr);
        }
        if (!included)
        {
            continue;
        }

        if (decl->type_is< ast2_template_declaration >() || decl->type_is< ast2_function_declaration >())
        {
            continue;
        }

        type_symbol child = is_member ? type_symbol{submember{.of = input, .name = *name}} : type_symbol{subsymbol{.of = input, .name = *name}};

        if (decl->type_is< ast2_namespace_declaration >() || decl->type_is< ast2_struct_declaration >())
        {
            std::set< type_symbol > const ns_results = co_await rpnx::querygraph::request< list_unit_tests_query >(child);
            result.insert(ns_results.begin(), ns_results.end());
            continue;
        }

        if (!decl->type_is< ast2_test >())
        {
            continue;
        }

        if (co_await rpnx::querygraph::request< test_is_enabled_for_unit_testing_query >(child))
        {
            result.insert(child);
        }
    }

    co_return result;
}
