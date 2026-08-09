// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_CORTADO_INPUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_CORTADO_INPUT_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/asm_procedure_from_symbol.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/functanoid_return_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/global_is_per_thread.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/list_unit_tests.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_cortado_backend_options.hpp>
#include <quxlang/queries/output_cortado_input.hpp>
#include <quxlang/queries/indexed_source_bundle.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/unit_test_vmir.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <quxlang/queries/vm_procedure3.hpp>
#include <quxlang/queries/vmir_dependencies.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Query specification for Cortado runtime-closure aggregation. */
    struct output_cortado_input_spec
    {
        using query = output_cortado_input_query;
        using dependencies = rpnx::typelist< antestatal_static_value_query, asm_procedure_from_symbol_query, class_type_query, enum_info_query, flagset_info_query, functanoid_return_type_query, global_is_antestatal_static_query, global_is_per_thread_query, indexed_source_bundle_query, instanciation_query, list_unit_tests_query, lookup_query, output_binary_information_query, output_cortado_backend_options_query, struct_field_list_query, symboid_query, target_configuration_query, union_info_query, unit_test_vmir_query, variable_type_query, variant_info_query, vm_procedure3_query, direct_dependencies_query >;
    };

    /** Implements Cortado runtime-closure aggregation without physical layout queries. */
    auto output_cortado_input_impl(std::string input) -> rpnx::querygraph::coroutine< output_cortado_input_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_CORTADO_INPUT_SPEC_HEADER_GUARD
