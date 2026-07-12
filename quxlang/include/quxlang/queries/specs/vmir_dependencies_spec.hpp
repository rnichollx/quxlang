// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_VMIR_DEPENDENCIES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_VMIR_DEPENDENCIES_SPEC_HEADER_GUARD

#include <quxlang/queries/vmir_dependencies.hpp>
#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/static_test_vmir.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>
#include <quxlang/queries/unit_test_vmir.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct direct_dependencies_spec
    {
        using query = direct_dependencies_query;
        using dependencies = rpnx::typelist< antestatal_static_value_query, class_type_query, global_is_antestatal_static_query, instanciation_query, lookup_query, static_test_vmir_query, symboid_query, symbol_type_query, temploid_formal_ensig_query, unit_test_vmir_query, union_info_query, variable_type_query, variant_info_query, vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< direct_dependencies_spec > direct_dependencies_impl(direct_dependencies_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_VMIR_DEPENDENCIES_SPEC_HEADER_GUARD
