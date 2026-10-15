// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/list_static_tests_spec.hpp>

rpnx::querygraph::coroutine< quxlang::list_static_tests_spec > quxlang::list_static_tests_impl(type_symbol input)
{
    std::vector< subdeclaration > const& input_subdeclarations = co_await rpnx::querygraph::request< active_symboid_subdeclarations_query >(input);

    std::set< type_symbol > result;

    for (subdeclaration const& sdcl : input_subdeclarations)
    {
        declaration const* decl = nullptr;
        std::string const* name = nullptr;
        bool is_member = false;

        if (sdcl.type_is< global_subdeclaration >())
        {
            global_subdeclaration const& global = sdcl.as< global_subdeclaration >();
            decl = &global.decl;
            name = &global.name;
        }
        else if (sdcl.type_is< member_subdeclaration >())
        {
            member_subdeclaration const& member = sdcl.as< member_subdeclaration >();
            decl = &member.decl;
            name = &member.name;
            is_member = true;
        }
        else
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

        if (decl->type_is< ast2_namespace_declaration >() || decl->type_is< ast2_struct_declaration >() || decl->type_is< ast2_union_declaration >() || decl->type_is< ast2_variant_declaration >())
        {
            auto ns_results = co_await rpnx::querygraph::request< list_static_tests_query >(child);
            result.insert(ns_results.begin(), ns_results.end());
            continue;
        }

        if (!decl->type_is< ast2_test >())
        {

            continue;
        }

        if (co_await rpnx::querygraph::request< test_is_enabled_for_static_testing_query >(child))
        {
            result.insert(child);
        }
    }

    co_return result;
}
