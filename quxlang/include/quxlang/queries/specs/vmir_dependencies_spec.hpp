// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_VMIR_DEPENDENCIES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_VMIR_DEPENDENCIES_SPEC_HEADER_GUARD

#include <quxlang/queries/vmir_dependencies.hpp>
#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/static_test_vmir.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/unit_test_vmir.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct direct_dependencies_spec
    {
        using query = direct_dependencies_query;
        using dependencies = rpnx::typelist< antestatal_static_value_query, static_test_vmir_query, symboid_query, unit_test_vmir_query, variable_type_query, vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< direct_dependencies_spec > direct_dependencies_impl(direct_dependencies_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_VMIR_DEPENDENCIES_SPEC_HEADER_GUARD
