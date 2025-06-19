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

        member_subdeclaroid const& sdcl_2 = sdcl.as< member_subdeclaroid >();

        std::string const& name = sdcl_2.name;

        if (!sdcl_2.decl.type_is< ast2_static_test >())
        {
            continue;
        }

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

        result.insert(subsymbol{.of = input, .name = sdcl_2.name});
    }

    throw rpnx::unimplemented();
}