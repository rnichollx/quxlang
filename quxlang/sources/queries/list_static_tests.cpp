// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/list_static_tests_spec.hpp>



rpnx::querygraph::coroutine< quxlang::list_static_tests_spec > quxlang::list_static_tests_impl(type_symbol input)
{
    auto input_subdeclaroids = co_await rpnx::querygraph::request< symboid_subdeclaroids_query >(input);

    std::set< type_symbol > result;

    for (subdeclaroid const& sdcl : input_subdeclaroids)
    {

        if (!sdcl.type_is< global_subdeclaroid >())
        {
            continue;
        }

        global_subdeclaroid const& sdcl_2 = sdcl.as< global_subdeclaroid >();

        bool included = true;
        if (sdcl_2.include_if.has_value())
        {
            constexpr_input input_constexpr;
            input_constexpr.context = input;
            input_constexpr.expr = *sdcl_2.include_if;
            included = co_await rpnx::querygraph::request< constexpr_bool_query >(input_constexpr);
        }
        if (!included)
        {
            continue;
        }

        std::string const& name = sdcl_2.name;

        if (sdcl_2.decl.type_is< ast2_template_declaration >() || sdcl_2.decl.type_is< ast2_function_declaration >())
        {
            // We need to ignore templates, since we don't know what parameters to use
            // to instantiate them.
            // Functions cannot contain static tests, so we can ignore them as well.
            // TODO: Run static tests on functions if they are concrete.
            continue;
        }

        if (sdcl_2.decl.type_is< ast2_namespace_declaration >() || sdcl_2.decl.type_is< ast2_class_declaration >())
        {
            auto ns_results = co_await rpnx::querygraph::request< list_static_tests_query >(subsymbol{.of = input, .name = sdcl_2.name});
            result.insert(ns_results.begin(), ns_results.end());
            continue;
        }

        if (!sdcl_2.decl.type_is< ast2_static_test >())
        {

            continue;
        }

        result.insert(subsymbol{.of = input, .name = sdcl_2.name});
    }

    throw rpnx::unimplemented();
}