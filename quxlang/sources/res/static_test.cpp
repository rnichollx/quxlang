//
// Created by rnicholl on 5/21/25.
//

#include "quxlang/res/static_test.hpp"

#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_static_tests)
{
    auto input_subdeclaroids = co_await QUX_CO_DEP(symboid_subdeclaroids, (input_val));

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
            included = co_await QUX_CO_DEP(constexpr_bool, (input_constexpr));
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
            auto ns_results = co_await QUX_CO_DEP(list_static_tests, (subsymbol{.of = input, .name = sdcl_2.name}));
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