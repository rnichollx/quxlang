// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_OUTPUT_LLVM_CATALOG_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LLVM_CATALOG_HEADER_GUARD
#include <quxlang/queries/output_llvm_input.hpp>
namespace quxlang
{
    /** Identifies a component closure independently of its machine-code stepping. */
    struct llvm_component_query_input
    {
        std::string output_name;
        llvm_output_component component = llvm_output_component::early_init;
        RPNX_MEMBER_METADATA(llvm_component_query_input, output_name, component);
    };
    /** Query keys and ownership metadata needed to materialize an LLVM component. */
    struct llvm_component_catalog
    {
        /** Maximum construction-phase assignment count required by this component's descriptors. */
        std::size_t struct_phase_assignment_capacity = 1;
        type_symbol target_name;
        bool place_definitions_in_stepping_section = false;
        bool suffix_generated_function_symbols = false;
        bool definitions_are_coalescible = false;
        bool emit_process_entrypoint = true;
        llvm_backend::root_routine_emission root_routine = llvm_backend::root_routine_emission::definition;
        bool defines_compiler_builtin_objects = false;
        std::optional< output_kind > whole_module_output_kind;
        std::optional< std::string > executable_entry_symbol;
        std::optional< type_symbol > post_detect_functanoid;
        std::vector< llvm_backend::unit_test_entry > unit_tests;
        llvm_backend::unit_test_object_emission unit_test_objects = llvm_backend::unit_test_object_emission::external_declarations;
        std::map< llvm_backend::runtime_procedure_reference, type_symbol > runtime_procedures;
        std::map< type_symbol, std::string > procedure_linksymbols;
        std::set< type_symbol > extern_procedures;
        std::set< type_symbol > optional_extern_procedures;
        std::map< type_symbol, std::string > extern_procedure_libraries;
        std::map< type_symbol, std::string > extern_procedure_versions;
        std::map< type_symbol, type_symbol > object_reference_types;
        std::map< type_symbol, initialization_type > global_init_types;
        std::set< type_symbol > assembly_referenced_procedures;
        std::set< type_symbol > inlinable_functions;
        std::set< type_symbol > asm_callable_interfaces;
        std::set< type_symbol > asm_functions;
        std::set< type_symbol > antestatal_constants;
        std::set< type_symbol > interface_slots;
        std::set< type_symbol > enum_infos;
        std::set< type_symbol > flagset_infos;
        std::set< type_symbol > struct_layouts;
        std::set< type_symbol > struct_runtime_infos;
        std::set< type_symbol > union_infos;
        std::set< type_symbol > variant_infos;
        std::set< type_symbol > fusion_layouts;
        std::set< type_symbol > type_placements;
        std::set< type_symbol > materialized_types;
        std::map< std::string, type_symbol > attribute_detectors;
        RPNX_MEMBER_METADATA(llvm_component_catalog, struct_phase_assignment_capacity, target_name, place_definitions_in_stepping_section, suffix_generated_function_symbols, definitions_are_coalescible, emit_process_entrypoint, root_routine, defines_compiler_builtin_objects, whole_module_output_kind, executable_entry_symbol, post_detect_functanoid, unit_tests, unit_test_objects, runtime_procedures, procedure_linksymbols, extern_procedures, optional_extern_procedures, extern_procedure_libraries, extern_procedure_versions, object_reference_types, global_init_types, assembly_referenced_procedures, inlinable_functions, asm_callable_interfaces, asm_functions, antestatal_constants, interface_slots, enum_infos, flagset_infos, struct_layouts, struct_runtime_infos, union_infos, variant_infos, fusion_layouts, type_placements, materialized_types, attribute_detectors);
    };
    /** Discovers reachable query identities without retaining their lowering payloads. */
    struct output_llvm_catalog_query
    {
        static constexpr auto query_id = "output_llvm_catalog";
        using input_type = llvm_component_query_input;
        using output_type = llvm_component_catalog;
    };
} // namespace quxlang
#endif
