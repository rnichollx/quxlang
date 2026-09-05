// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_PREOPTIMIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_PREOPTIMIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/indexed_source_bundle.hpp>
#include <quxlang/queries/llvm_compiler_builtin_manifest.hpp>
#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/llvm_preoptimize.hpp>
#include <quxlang/queries/output_llvm_backend_options.hpp>
#include <quxlang/queries/output_llvm_input.hpp>
#include <quxlang/queries/output_llvm_type_ordinals.hpp>
#include <quxlang/queries/specs/output_llvm_catalog_spec.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_preoptimize_spec
    {
        using query = llvm_preoptimize_query;
        using dependencies = rpnx::typelist< output_llvm_catalog_query, direct_dependencies_query, output_llvm_backend_options_query, output_steppings_query, machine_info_query, unit_test_vmir_query, vm_procedure3_query, asm_procedure_from_symbol_query, antestatal_static_value_query, class_placement_info_query, indexed_source_bundle_query, interface_slot_list_query, enum_info_query, flagset_info_query, struct_layout_query, struct_runtime_info_query, union_info_query, variant_info_query, fusion_layout_query, output_llvm_type_ordinals_query, llvm_output_component_identities_query, llvm_compiler_builtin_manifest_query >;
    };

    rpnx::querygraph::coroutine< llvm_preoptimize_spec > llvm_preoptimize_impl(llvm_output_query_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_PREOPTIMIZE_SPEC_HEADER_GUARD
