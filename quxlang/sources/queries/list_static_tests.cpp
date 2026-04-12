// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/list_static_tests_spec.hpp>



rpnx::querygraph::coroutine< quxlang::list_static_tests_spec > quxlang::list_static_tests_impl(type_symbol input)
{
    auto input_subdeclaroids = co_await rpnx::querygraph::request< symboid_subdeclaroids_query >(input);

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
            // We need to ignore templates, since we don't know what parameters to use
            // to instantiate them.
            // Functions cannot contain static tests, so we can ignore them as well.
            // TODO: Run static tests on functions if they are concrete.
            continue;
        }

        type_symbol child = is_member ? type_symbol{submember{.of = input, .name = *name}} : type_symbol{subsymbol{.of = input, .name = *name}};

        if (decl->type_is< ast2_namespace_declaration >() || decl->type_is< ast2_class_declaration >())
        {
            auto ns_results = co_await rpnx::querygraph::request< list_static_tests_query >(child);
            result.insert(ns_results.begin(), ns_results.end());
            continue;
        }

        if (!decl->type_is< ast2_static_test >())
        {

            continue;
        }

        result.insert(child);
    }

    co_return result;
}
