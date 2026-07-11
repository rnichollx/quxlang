// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_INPUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_INPUT_SPEC_HEADER_GUARD

#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/asm_procedure_from_symbol.hpp>
#include <quxlang/queries/struct_layout.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/functanoid_return_type.hpp>
#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/list_unit_tests.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_llvm_backend_options.hpp>
#include <quxlang/queries/output_llvm_input.hpp>
#include <quxlang/queries/procedure_linksymbol.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/target_llvm_backend_options.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>
#include <quxlang/queries/class_placement_info.hpp>
#include <quxlang/queries/unit_test_vmir.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_llvm_input_spec
    {
        using query = output_llvm_input_query;
        using dependencies = rpnx::typelist<
            antestatal_static_value_query,
            asm_procedure_from_symbol_query,
            struct_layout_query,
            enum_info_query,
            flagset_info_query,
            functanoid_return_type_query,
            global_init_type_query,
            global_is_antestatal_static_query,
            interface_slot_list_query,
            instanciation_query,
            list_unit_tests_query,
            lookup_query,
            machine_info_query,
            output_binary_information_query,
            procedure_linksymbol_query,
            source_bundle_query,
            source_file_index_query,
            symboid_query,
            class_type_query, symbol_type_query,
            target_configuration_query,
            temploid_formal_ensig_query,
            output_llvm_backend_options_query,
            class_placement_info_query,
            unit_test_vmir_query,
            variable_type_query,
            vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< output_llvm_input_spec > output_llvm_input_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_INPUT_SPEC_HEADER_GUARD
