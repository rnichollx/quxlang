cmake_minimum_required(VERSION 4.2)
project(Quxlang)

include(GNUInstallDirs)
include(CTest)
if (BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)
endif ()

find_package(yaml-cpp REQUIRED)
find_package(RPNXMetalib REQUIRED)
find_package(RPNXSerialization REQUIRED)
find_package(RPNXDataStructures REQUIRED)
find_package(RPNXQueryGraph REQUIRED)
find_package(LLVM REQUIRED)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")


find_package(RPNXCow REQUIRED)
find_package(benchmark REQUIRED)

# Legacy LLVM backend / VMIR v1 sources remain in the tree, but are not built.
add_library(quxlang include/quxlang/fwd.hpp include/quxlang/data/fwd.hpp include/quxlang/data/entity_category.hpp include/quxlang/fwd.hpp include/quxlang/data/symbol_id.hpp include/quxlang/data/lookup_type.hpp include/quxlang/data/lookup_chain.hpp include/quxlang/data/lookup_singular.hpp include/quxlang/manipulators/merge_entity.hpp sources/merge_entity.cpp include/quxlang/data/contextual_type_reference.hpp include/quxlang/data/class_field_declaration.hpp include/quxlang/data/absolute_lookup_reference.hpp include/quxlang/data/proximate_lookup_reference.hpp include/quxlang/data/class_layout.hpp include/quxlang/data/class_field_info.hpp include/quxlang/data/enum_flagset_info.hpp include/quxlang/data/lambda_types.hpp include/quxlang/data/type_placement_info.hpp include/quxlang/manipulators/struct_math.hpp include/quxlang/data/basic_types.hpp include/quxlang/data/function_block.hpp include/quxlang/data/function_statement.hpp include/quxlang/data/function_if_statement.hpp include/quxlang/data/function_expression_statement.hpp include/quxlang/cow.hpp include/quxlang/manipulators/expression_stringifier.hpp include/quxlang/data/canonical_resolved_function_chain.hpp include/quxlang/data/function_argument.hpp include/quxlang/data/cpu_architecture.hpp include/quxlang/data/function_return_statement.hpp include/quxlang/ordering.hpp include/quxlang/manipulators/mangler.hpp include/quxlang/converters/qual_converters.hpp include/quxlang/manipulators/typeutils.hpp
        include/quxlang/variant_utils.hpp
        include/quxlang/vmir2/vmir2.hpp
        include/quxlang/data/numeric_literal.hpp
        include/quxlang/data/lookup_type.hpp
        sources/manipulators/typeutils.cpp
        include/quxlang/manipulators/numeric_literal_utils.hpp
        include/quxlang/data/function_while_statement.hpp
        include/quxlang/data/symbol_type.hpp
        include/quxlang/operators.hpp
        include/quxlang/data/function_delegate.hpp
        include/quxlang/ast2/ast2_type_map.hpp
        include/quxlang/ast2/ast2_entity.hpp
        include/quxlang/ast2/ast2_function_delegate.hpp
        include/quxlang/parsers/parse_class_body.hpp
        include/quxlang/parsers/try_parse_class.hpp
        include/quxlang/parsers/try_parse_enum_flagset.hpp
        include/quxlang/parsers/try_parse_interface.hpp
        include/quxlang/parsers/try_parse_template_declaration.hpp
        include/quxlang/parsers/skip_whitespace.hpp
        include/quxlang/parsers/keyword.hpp
        include/quxlang/parsers/iter_parse_symbol.hpp
        include/quxlang/parsers/try_parse_class_variable_declaration.hpp
        include/quxlang/parsers/parse_comment.hpp
        include/quxlang/parsers/iter_parse_line_comment.hpp
        include/quxlang/parsers/parse_symbol.hpp
        include/quxlang/parsers/parse_whitespace_and_comments.hpp
        include/quxlang/parsers/ctype.hpp
        include/quxlang/parsers/parse_whitespace.hpp
        include/quxlang/parsers/try_parse_function_declaration.hpp
        include/quxlang/parsers/try_parse_type_symbol.hpp
        include/quxlang/parsers/try_parse_integral_keyword.hpp
        include/quxlang/parsers/try_parse_class_function_declaration.hpp
        include/quxlang/parsers/parse_type_symbol.hpp
        include/quxlang/parsers/parse_function_block.hpp
        include/quxlang/parsers/parse_function_declaration.hpp
        include/quxlang/parsers/parse_function_args.hpp
        include/quxlang/ast2/ast2_function_arg.hpp
        include/quxlang/parsers/try_parse_function_return_type.hpp
        include/quxlang/parsers/try_parse_statement.hpp
        include/quxlang/parsers/parse_if_statement.hpp
        include/quxlang/parsers/parse_expression.hpp
        include/quxlang/parsers/try_parse_expression.hpp
        include/quxlang/parsers/parse_int.hpp
        include/quxlang/parsers/parse_var_statement.hpp
        include/quxlang/parsers/parse_keyword.hpp
        include/quxlang/parsers/try_parse_expression_statement.hpp
        include/quxlang/parsers/parse_return_statement.hpp
        include/quxlang/parsers/parse_while_statement.hpp
        include/quxlang/parsers/iter_parse_number.hpp
        include/quxlang/parsers/try_parse_function_callsite_expression.hpp
        include/quxlang/parsers/parse_class.hpp
        include/quxlang/ast2/ast2_namespace.hpp
        include/quxlang/ast2/ast2manip.hpp
        include/quxlang/ast2/ast2_module.hpp
        include/quxlang/parsers/parse_file.hpp
        include/quxlang/parsers/parse_file.hpp
        include/quxlang/parsers/try_parse_declaration.hpp
        include/quxlang/parsers/try_parse_variable_declaration.hpp
        include/quxlang/parsers/try_parse_name.hpp
        include/quxlang/ast2/ast2_named_declaration.hpp
        include/quxlang/parsers/try_parse_named_declaration.hpp
        include/quxlang/parsers/try_parse_namespace.hpp
        include/quxlang/parsers/parse_identifier.hpp
        include/quxlang/parsers/iter_parse_identifier.hpp
        include/quxlang/parsers/parse_subentity.hpp
        include/quxlang/parsers/peek_symbol.hpp
        include/quxlang/parsers/try_parse_function_delegates.hpp
        include/quxlang/parsers/try_parse_function_callsite_args.hpp
        include/quxlang/parsers/try_parse_delegate_callsite_args.hpp
        sources/ast2_entity.cpp
        include/quxlang/data/temploid_instanciation_parameter_set.hpp
        include/quxlang/ast2/ast2_externfunc.hpp
        include/quxlang/data/output_object_symbol.hpp
        include/quxlang/data/code_relocation.hpp
        include/quxlang/macros.hpp
        include/quxlang/manipulators/symbolmap.hpp
        include/quxlang/data/machine.hpp
        include/quxlang/input_aggregator.hpp
        sources/input_aggregator.cpp
        include/quxlang/linker/elf_linker.hpp
        include/quxlang/parsers/asm/arm_assembler.hpp
        include/quxlang/asm/asm.hpp
        include/quxlang/parsers/parse_asm_procedure.hpp
        include/quxlang/backends/asm/arm_asm_converter.hpp
        include/quxlang/backends/asm/x64_asm_converter.hpp
        include/quxlang/parsers/asm/asm_callable.hpp
        include/quxlang/parsers/asm/parse_register.hpp
        include/quxlang/backends/elf/elf.hpp
        include/quxlang/parsers/import.hpp
        include/quxlang/parsers/extern.hpp
        include/quxlang/parsers/string_literal.hpp
        include/quxlang/parsers/symbol.hpp
        include/quxlang/parsers/procedure_ref.hpp
        include/quxlang/data/target_configuration.hpp
        include/quxlang/data/interp_value.hpp
        include/quxlang/parsers/declaration.hpp
        include/quxlang/parsers/doc.hpp
        include/quxlang/data/variants.hpp
        include/quxlang/parsers/include_if.hpp
        include/quxlang/parsers/expression/target.hpp
        include/quxlang/data/function_header.hpp
        include/quxlang/data/fwd.hpp
        include/quxlang/builtins.hpp
        include/quxlang/data/builtin_functions.hpp
        include/quxlang/vmir2/assembler.hpp
        sources/vmir2/assembler.cpp
        include/quxlang/vmir2/routine_requirements.hpp
        sources/vmir2/routine_requirements.cpp
        include/quxlang/vmir2/ir2_constexpr_interpreter.hpp
        sources/ir2_constexpr_interpreter.cpp
        include/quxlang/exception.hpp
        include/quxlang/vmir2/state_engine.hpp
        include/quxlang/lang/lang.hpp
        include/quxlang/bytemath.hpp
        include/quxlang/ast2/source_location.hpp
        sources/bytemath.cpp
        include/quxlang/data/parser_data.hpp
        include/quxlang/keywords.hpp
        include/quxlang/fixed_bytemath.hpp
        include/quxlang/parsers/option.hpp
        include/quxlang/data/statements.hpp
        include/quxlang/data/compilation_result.hpp
        include/quxlang/parsers/context.hpp
        include/quxlang/co_vmir_generator2_deps.hpp
        include/quxlang/data/basic_types.hpp
        include/quxlang/llvm-backend.hpp
        sources/llvm-backend.cpp
        include/quxlang/linker/elf_linker.hpp
        sources/linker/elf_linker.cpp
        include/quxlang/profiling.hpp
        include/quxlang/llvm-backend-types.hpp
)
add_library(Quxlang::quxlang ALIAS quxlang)

llvm_map_components_to_libnames(QUXLANG_LLVM_LIBS Core Support BitWriter Passes TransformUtils Target CodeGen MC MCParser AsmParser AsmPrinter Object X86 AArch64 ARM RISCV)


add_library(quxlang-loaders
        sources/loaders/source_loader.cpp

        include/quxlang/source_loader.hpp)
add_library(Quxlang::quxlang-loaders ALIAS quxlang-loaders)
target_link_libraries(quxlang-loaders PUBLIC quxlang yaml-cpp::yaml-cpp)


target_include_directories(quxlang PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(quxlang SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(quxlang-loaders PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(quxlang PUBLIC cxx_std_23)
target_compile_options(quxlang PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fconstexpr-steps=67108864>)
target_compile_options(quxlang PRIVATE ${LLVM_DEFINITIONS})
target_compile_definitions(quxlang PUBLIC QUXLANG_DEBUG_MESSAGES_ENABLED=false)
target_link_libraries(quxlang PRIVATE ${QUXLANG_LLVM_LIBS})
set_target_properties(quxlang PROPERTIES EXPORT_NAME quxlang)
set_target_properties(quxlang-loaders PROPERTIES EXPORT_NAME quxlang-loaders)

file(GLOB QUXLANG_QUERY_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/quxlang/queries/*.hpp")
set(QUXLANG_QUERY_SPEC_HEADERS
        include/quxlang/queries/specs/argument_adaptation_is_better_fit_spec.hpp
        include/quxlang/queries/specs/argument_adaptation_rank_spec.hpp
        include/quxlang/queries/specs/argument_initialize_by_class_conversion_spec.hpp
        include/quxlang/queries/specs/argument_initialize_by_intrinsic_spec.hpp
        include/quxlang/queries/specs/argument_initialize_by_template_spec.hpp
        include/quxlang/queries/specs/antestatal_static_value_spec.hpp
        include/quxlang/queries/specs/asm_procedure_from_symbol_spec.hpp
        include/quxlang/queries/specs/bindable_by_reference_objectization_spec.hpp
        include/quxlang/queries/specs/bindable_by_reference_requalification_spec.hpp
        include/quxlang/queries/specs/bindable_by_temporary_materialization_spec.hpp
        include/quxlang/queries/specs/bindable_spec.hpp
        include/quxlang/queries/specs/builtin_assignment_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/builtin_copy_ctor_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/builtin_datatype_compare_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/builtin_default_ctor_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/builtin_dtor_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/builtin_move_ctor_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/builtin_swap_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/builtin_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/class_builtin_spec.hpp
        include/quxlang/queries/specs/class_default_ctor_spec.hpp
        include/quxlang/queries/specs/class_default_dtor_spec.hpp
        include/quxlang/queries/specs/class_field_declaration_list_spec.hpp
        include/quxlang/queries/specs/class_field_list_spec.hpp
        include/quxlang/queries/specs/class_layout_spec.hpp
        include/quxlang/queries/specs/class_requires_gen_assignment_spec.hpp
        include/quxlang/queries/specs/class_requires_gen_copy_ctor_spec.hpp
        include/quxlang/queries/specs/class_requires_gen_default_ctor_spec.hpp
        include/quxlang/queries/specs/class_requires_gen_default_dtor_spec.hpp
        include/quxlang/queries/specs/class_requires_gen_move_ctor_spec.hpp
        include/quxlang/queries/specs/class_requires_gen_swap_spec.hpp
        include/quxlang/queries/specs/class_tags_spec.hpp
        include/quxlang/queries/specs/class_trivially_constructible_spec.hpp
        include/quxlang/queries/specs/class_trivially_destructible_spec.hpp
        include/quxlang/queries/specs/constexpr_bool_spec.hpp
        include/quxlang/queries/specs/constexpr_eval_antestatal_spec.hpp
        include/quxlang/queries/specs/constexpr_eval_string_spec.hpp
        include/quxlang/queries/specs/constexpr_eval_numeric_spec.hpp
        include/quxlang/queries/specs/constexpr_eval_v3_spec.hpp
        include/quxlang/queries/specs/constexpr_eval_spec.hpp
        include/quxlang/queries/specs/constexpr_routine_antestatal_spec.hpp
        include/quxlang/queries/specs/constexpr_routine_v3_spec.hpp
        include/quxlang/queries/specs/constexpr_routine_spec.hpp
        include/quxlang/queries/specs/constexpr_u64_spec.hpp
        include/quxlang/queries/specs/convertible_by_call_spec.hpp
        include/quxlang/queries/specs/declaroids_spec.hpp
        include/quxlang/queries/specs/ensig_argument_initialize_spec.hpp
        include/quxlang/queries/specs/ensig_tempars_spec.hpp
        include/quxlang/queries/specs/enum_info_spec.hpp
        include/quxlang/queries/specs/exists_spec.hpp
        include/quxlang/queries/specs/extern_linksymbol_spec.hpp
        include/quxlang/queries/specs/flagset_info_spec.hpp
        include/quxlang/queries/specs/functanoid_directly_instantiated_functanoids_spec.hpp
        include/quxlang/queries/specs/functanoid_required_class_layouts_spec.hpp
        include/quxlang/queries/specs/functanoid_required_type_placements_spec.hpp
        include/quxlang/queries/specs/functanoid_return_type_spec.hpp
        include/quxlang/queries/specs/functanoid_sigtype_spec.hpp
        include/quxlang/queries/specs/function_builtin_spec.hpp
        include/quxlang/queries/specs/function_declaration_spec.hpp
        include/quxlang/queries/specs/function_ensig_init_with_spec.hpp
        include/quxlang/queries/specs/function_instanciation_spec.hpp
        include/quxlang/queries/specs/function_pack_info_spec.hpp
        include/quxlang/queries/specs/function_param_names_spec.hpp
        include/quxlang/queries/specs/function_positional_parameter_names_spec.hpp
        include/quxlang/queries/specs/function_primitive_spec.hpp
        include/quxlang/queries/specs/functum_builtin_overloads_spec.hpp
        include/quxlang/queries/specs/functum_builtins_spec.hpp
        include/quxlang/queries/specs/functum_exists_and_is_callable_with_spec.hpp
        include/quxlang/queries/specs/functum_initialize_spec.hpp
        include/quxlang/queries/specs/functum_list_user_ensig_declarations_spec.hpp
        include/quxlang/queries/specs/functum_list_user_overload_declarations_spec.hpp
        include/quxlang/queries/specs/functum_map_user_formal_ensigs_spec.hpp
        include/quxlang/queries/specs/functum_overloads_spec.hpp
        include/quxlang/queries/specs/functum_select_function_spec.hpp
        include/quxlang/queries/specs/functum_user_overloads_spec.hpp
        include/quxlang/queries/specs/global_init_type_spec.hpp
        include/quxlang/queries/specs/global_is_antestatal_static_spec.hpp
        include/quxlang/queries/specs/global_is_per_thread_spec.hpp
        include/quxlang/queries/specs/global_is_serialoid_static_spec.hpp
        include/quxlang/queries/specs/global_is_string_static_spec.hpp
        include/quxlang/queries/specs/global_is_numeric_static_spec.hpp
        include/quxlang/queries/specs/have_nontrivial_member_ctor_spec.hpp
        include/quxlang/queries/specs/have_nontrivial_member_dtor_spec.hpp
        include/quxlang/queries/specs/implementation_function_map_spec.hpp
        include/quxlang/queries/specs/implementation_interface_type_spec.hpp
        include/quxlang/queries/specs/implicitly_convertible_to_spec.hpp
        include/quxlang/queries/specs/instanciation_spec.hpp
        include/quxlang/queries/specs/instanciation_tempar_map_spec.hpp
        include/quxlang/queries/specs/interpret_bool_spec.hpp
        include/quxlang/queries/specs/interpret_value_spec.hpp
        include/quxlang/queries/specs/interface_defaultable_spec.hpp
        include/quxlang/queries/specs/interface_slot_list_spec.hpp
        include/quxlang/queries/specs/llvm_compiled_output_spec.hpp
        include/quxlang/queries/specs/llvm_output_binary_artifact_spec.hpp
        include/quxlang/queries/specs/output_binaries_information_spec.hpp
        include/quxlang/queries/specs/output_binary_artifact_spec.hpp
        include/quxlang/queries/specs/output_binary_artifacts_spec.hpp
        include/quxlang/queries/specs/output_binary_information_spec.hpp
        include/quxlang/queries/specs/output_llvm_backend_options_spec.hpp
        include/quxlang/queries/specs/output_llvm_input_spec.hpp
        include/quxlang/queries/specs/output_list_spec.hpp
        include/quxlang/queries/specs/output_optimized_llvm_spec.hpp
        include/quxlang/queries/specs/output_unoptimized_llvm_spec.hpp
        include/quxlang/queries/specs/target_backend_spec.hpp
        include/quxlang/queries/specs/target_llvm_backend_options_spec.hpp
        include/quxlang/queries/specs/list_builtin_constructors_spec.hpp
        include/quxlang/queries/specs/list_static_tests_spec.hpp
        include/quxlang/queries/specs/list_unit_tests_spec.hpp
        include/quxlang/queries/specs/list_user_functum_formal_paratypes_spec.hpp
        include/quxlang/queries/specs/lookup_spec.hpp
        include/quxlang/queries/specs/module_ast_spec.hpp
        include/quxlang/queries/specs/module_source_name_spec.hpp
        include/quxlang/queries/specs/module_sources_spec.hpp
        include/quxlang/queries/specs/parse_file_spec.hpp
        include/quxlang/queries/specs/procedure_linksymbol_spec.hpp
        include/quxlang/queries/specs/run_static_test_spec.hpp
        include/quxlang/queries/specs/run_static_tests_spec.hpp
        include/quxlang/queries/specs/sintpointer_type_spec.hpp
        include/quxlang/queries/specs/serialoid_static_value_spec.hpp
        include/quxlang/queries/specs/source_file_id_spec.hpp
        include/quxlang/queries/specs/source_file_index_spec.hpp
        include/quxlang/queries/specs/source_file_name_spec.hpp
        include/quxlang/queries/specs/static_test_vmir_spec.hpp
        include/quxlang/queries/specs/string_static_value_spec.hpp
        include/quxlang/queries/specs/numeric_static_value_spec.hpp
        include/quxlang/queries/specs/symboid_spec.hpp
        include/quxlang/queries/specs/symboid_subdeclaroids_spec.hpp
        include/quxlang/queries/specs/symbol_tempars_spec.hpp
        include/quxlang/queries/specs/symbol_type_spec.hpp
        include/quxlang/queries/specs/template_instanciation_spec.hpp
        include/quxlang/queries/specs/test_is_enabled_for_static_testing_spec.hpp
        include/quxlang/queries/specs/test_is_enabled_for_unit_testing_spec.hpp
        include/quxlang/queries/specs/temploid_formal_ensig_spec.hpp
        include/quxlang/queries/specs/templex_initialize_spec.hpp
        include/quxlang/queries/specs/type_is_antestatal_spec.hpp
        include/quxlang/queries/specs/type_is_serialoid_spec.hpp
        include/quxlang/queries/specs/type_is_stringlike_spec.hpp
        include/quxlang/queries/specs/type_is_implicitly_datatype_spec.hpp
        include/quxlang/queries/specs/type_is_trivially_default_constructible_spec.hpp
        include/quxlang/queries/specs/type_is_trivially_relocatable_spec.hpp
        include/quxlang/queries/specs/type_placement_info_spec.hpp
        include/quxlang/queries/specs/type_should_autogen_deserialize_spec.hpp
        include/quxlang/queries/specs/type_should_autogen_serialize_spec.hpp
        include/quxlang/queries/specs/unit_test_vmir_spec.hpp
        include/quxlang/queries/specs/uintpointer_type_spec.hpp
        include/quxlang/queries/specs/user_assignment_exists_spec.hpp
        include/quxlang/queries/specs/user_copy_ctor_exists_spec.hpp
        include/quxlang/queries/specs/user_default_ctor_exists_spec.hpp
        include/quxlang/queries/specs/user_default_dtor_exists_spec.hpp
        include/quxlang/queries/specs/user_deserialize_exists_spec.hpp
        include/quxlang/queries/specs/user_move_ctor_exists_spec.hpp
        include/quxlang/queries/specs/user_serialize_exists_spec.hpp
        include/quxlang/queries/specs/user_swap_exists_spec.hpp
        include/quxlang/queries/specs/user_vm_procedure3_spec.hpp
        include/quxlang/queries/specs/variable_type_spec.hpp
        include/quxlang/queries/specs/vm_procedure3_spec.hpp
)

set(QUXLANG_QUERY_SOURCES
        sources/queries/argument_adaptation_is_better_fit.cpp
        sources/queries/argument_adaptation_rank.cpp
        sources/queries/argument_initialize_by_class_conversion.cpp
        sources/queries/argument_initialize_by_intrinsic.cpp
        sources/queries/argument_initialize_by_template.cpp
        sources/queries/antestatal_static_value.cpp
        sources/queries/asm_procedure_from_symbol.cpp
        sources/queries/bindable.cpp
        sources/queries/bindable_by_reference_objectization.cpp
        sources/queries/bindable_by_reference_requalification.cpp
        sources/queries/bindable_by_temporary_materialization.cpp
        sources/queries/builtin_assignment_vm_procedure3.cpp
        sources/queries/builtin_copy_ctor_vm_procedure3.cpp
        sources/queries/builtin_datatype_compare_vm_procedure3.cpp
        sources/queries/builtin_default_ctor_vm_procedure3.cpp
        sources/queries/builtin_dtor_vm_procedure3.cpp
        sources/queries/builtin_move_ctor_vm_procedure3.cpp
        sources/queries/builtin_swap_vm_procedure3.cpp
        sources/queries/builtin_template_instanciation.cpp
        sources/queries/builtin_vm_procedure3.cpp
        sources/queries/class_builtin.cpp
        sources/queries/class_default_ctor.cpp
        sources/queries/class_default_dtor.cpp
        sources/queries/class_field_declaration_list.cpp
        sources/queries/class_field_list.cpp
        sources/queries/class_layout.cpp
        sources/queries/class_requires_gen_assignment.cpp
        sources/queries/class_requires_gen_copy_ctor.cpp
        sources/queries/class_requires_gen_default_ctor.cpp
        sources/queries/class_requires_gen_default_dtor.cpp
        sources/queries/class_requires_gen_move_ctor.cpp
        sources/queries/class_requires_gen_swap.cpp
        sources/queries/class_tags.cpp
        sources/queries/class_trivially_constructible.cpp
        sources/queries/class_trivially_destructible.cpp
        sources/queries/constexpr_bool.cpp
        sources/queries/constexpr_eval_antestatal.cpp
        sources/queries/constexpr_eval_string.cpp
        sources/queries/constexpr_eval_numeric.cpp
        sources/queries/constexpr_eval.cpp
        sources/queries/constexpr_routine_antestatal.cpp
        sources/queries/constexpr_routine.cpp
        sources/queries/constexpr_u64.cpp
        sources/queries/convertible_by_call.cpp
        sources/queries/declaroids.cpp
        sources/queries/ensig_argument_initialize.cpp
        sources/queries/ensig_initialize.cpp
        sources/queries/ensig_tempars.cpp
        sources/queries/enum_info.cpp
        sources/queries/exists.cpp
        sources/queries/extern_linksymbol.cpp
        sources/queries/flagset_info.cpp
        sources/queries/functanoid_directly_instantiated_functanoids.cpp
        sources/queries/functanoid_required_class_layouts.cpp
        sources/queries/functanoid_required_type_placements.cpp
        sources/queries/functanoid_return_type.cpp
        sources/queries/functanoid_sigtype.cpp
        sources/queries/function_builtin.cpp
        sources/queries/function_declaration.cpp
        sources/queries/function_ensig_init_with.cpp
        sources/queries/function_instanciation.cpp
        sources/queries/function_pack_info.cpp
        sources/queries/function_param_names.cpp
        sources/queries/function_positional_parameter_names.cpp
        sources/queries/function_primitive.cpp
        sources/queries/functum_builtin_overloads.cpp
        sources/queries/functum_builtins.cpp
        sources/queries/functum_exists_and_is_callable_with.cpp
        sources/queries/functum_initialize.cpp
        sources/queries/functum_list_user_ensig_declarations.cpp
        sources/queries/functum_list_user_overload_declarations.cpp
        sources/queries/functum_map_user_formal_ensigs.cpp
        sources/queries/functum_overloads.cpp
        sources/queries/functum_select_function.cpp
        sources/queries/functum_user_overloads.cpp
        sources/queries/global_init_type.cpp
        sources/queries/global_is_antestatal_static.cpp
        sources/queries/global_is_per_thread.cpp
        sources/queries/global_is_serialoid_static.cpp
        sources/queries/global_is_string_static.cpp
        sources/queries/global_is_numeric_static.cpp
        sources/queries/have_nontrivial_member_ctor.cpp
        sources/queries/have_nontrivial_member_dtor.cpp
        sources/queries/implementation_function_map.cpp
        sources/queries/implementation_interface_type.cpp
        sources/queries/implicitly_convertible_to.cpp
        sources/queries/instanciation.cpp
        sources/queries/instanciation_concrete_params.cpp
        sources/queries/instanciation_tempar_map.cpp
        sources/queries/interpret_bool.cpp
        sources/queries/interpret_value.cpp
        sources/queries/interface_defaultable.cpp
        sources/queries/interface_slot_list.cpp
        sources/queries/list_builtin_constructors.cpp
        sources/queries/list_static_tests.cpp
        sources/queries/list_unit_tests.cpp
        sources/queries/list_user_functum_formal_paratypes.cpp
        sources/queries/lookup.cpp
        sources/queries/module_ast.cpp
        sources/queries/module_options_map.cpp
        sources/queries/module_source_name.cpp
        sources/queries/module_sources.cpp
        sources/queries/llvm_compiled_output.cpp
        sources/queries/llvm_output_binary_artifact.cpp
        sources/queries/output_binaries_information.cpp
        sources/queries/output_binary_artifact.cpp
        sources/queries/output_binary_artifacts.cpp
        sources/queries/output_binary_information.cpp
        sources/queries/output_llvm_backend_options.cpp
        sources/queries/output_llvm_input.cpp
        sources/queries/output_list.cpp
        sources/queries/output_optimized_llvm.cpp
        sources/queries/output_unoptimized_llvm.cpp
        sources/queries/target_backend.cpp
        sources/queries/target_llvm_backend_options.cpp
        sources/queries/parse_file.cpp
        sources/queries/procedure_linksymbol.cpp
        sources/queries/run_static_test.cpp
        sources/queries/run_static_tests.cpp
        sources/queries/serialoid_static_value.cpp
        sources/queries/source_file_id.cpp
        sources/queries/source_file_index.cpp
        sources/queries/source_file_name.cpp
        sources/queries/static_test_vmir.cpp
        sources/queries/string_static_value.cpp
        sources/queries/numeric_static_value.cpp
        sources/queries/sintpointer_type.cpp
        sources/queries/symboid.cpp
        sources/queries/symboid_subdeclaroids.cpp
        sources/queries/symbol_tempars.cpp
        sources/queries/symbol_type.cpp
        sources/queries/template_builtin.cpp
        sources/queries/template_instanciation.cpp
        sources/queries/test_is_enabled_for_static_testing.cpp
        sources/queries/test_is_enabled_for_unit_testing.cpp
        sources/queries/temploid_formal_ensig.cpp
        sources/queries/templex_builtins.cpp
        sources/queries/unit_test_vmir.cpp
        sources/queries/templex_builtin_templates.cpp
        sources/queries/templex_initialize.cpp
        sources/queries/templex_select_template.cpp
        sources/queries/type_is_antestatal.cpp
        sources/queries/type_is_serialoid.cpp
        sources/queries/type_is_stringlike.cpp
        sources/queries/type_is_implicitly_datatype.cpp
        sources/queries/type_is_trivially_default_constructible.cpp
        sources/queries/type_is_trivially_relocatable.cpp
        sources/queries/type_placement_info.cpp
        sources/queries/type_should_autogen_deserialize.cpp
        sources/queries/type_should_autogen_serialize.cpp
        sources/queries/uintpointer_type.cpp
        sources/queries/user_assignment_exists.cpp
        sources/queries/user_copy_ctor_exists.cpp
        sources/queries/user_default_ctor_exists.cpp
        sources/queries/user_default_dtor_exists.cpp
        sources/queries/user_deserialize_exists.cpp
        sources/queries/user_move_ctor_exists.cpp
        sources/queries/user_serialize_exists.cpp
        sources/queries/user_swap_exists.cpp
        sources/queries/user_vm_procedure3.cpp
        sources/queries/variable_type.cpp
        sources/queries/vm_procedure3.cpp
)


target_sources(quxlang PRIVATE sources/compiler_querygraph_01.cpp
        sources/compiler_querygraph_02.cpp
        sources/compiler_querygraph_03.cpp
        sources/compiler_querygraph_04.cpp
        sources/compiler_querygraph_05.cpp
        sources/compiler_querygraph_06.cpp
        sources/compiler_querygraph_07.cpp
        sources/compiler_querygraph_08.cpp
        sources/compiler_querygraph_09.cpp
        sources/compiler_querygraph_10.cpp
        sources/compiler_querygraph_11.cpp
        sources/compiler_querygraph_12.cpp
        sources/compiler_querygraph_13.cpp
        sources/compiler_querygraph_14.cpp
        sources/compiler_querygraph_15.cpp
        sources/compiler_querygraph_16.cpp
        sources/compiler_querygraph_17.cpp
        sources/compiler_querygraph_18.cpp
        sources/compiler_querygraph_19.cpp
        sources/compiler_querygraph_20.cpp
        sources/variant_instantiations.cpp
        ${QUXLANG_QUERY_SOURCES}
        include/quxlang/compiler_querygraph.hpp
        include/quxlang/co_vmir_generator2.hpp
        include/quxlang/co_vmir_generator2_deps.hpp
        include/quxlang/compiler_fwd.hpp
        include/quxlang/compiler_fwd.hpp
        include/quxlang/filelist.hpp
        ${QUXLANG_QUERY_HEADERS}
        ${QUXLANG_QUERY_SPEC_HEADERS}
        include/quxlang/data/functanoid_requirements.hpp
        include/quxlang/data/target_configuration.hpp
        include/quxlang/data/argument_adaptation_types.hpp
        include/quxlang/data/adaptation_types.hpp
        include/quxlang/data/class_tags_types.hpp
        include/quxlang/data/convertibility_types.hpp
        include/quxlang/data/constexpr_types.hpp
        include/quxlang/data/codegen_types.hpp
        include/quxlang/data/enum_flagset_info.hpp
        include/quxlang/data/functum_types.hpp
        include/quxlang/data/symbol_functum_types.hpp
        include/quxlang/data/tempar_types.hpp)
target_link_libraries(quxlang PUBLIC RPNX::cow RPNX::datastructures RPNX::querygraph RPNX::metalib)


add_executable(quxlang-benchmarks sources/benchmarks/variant_bms.cpp)

target_link_libraries(quxlang-benchmarks PRIVATE benchmark::benchmark benchmark::benchmark_main quxlang)

add_executable(quxlang-parser-benchmarks sources/benchmarks/parser_bms.cpp)

target_link_libraries(quxlang-parser-benchmarks PRIVATE benchmark::benchmark benchmark::benchmark_main quxlang)
target_compile_definitions(quxlang-parser-benchmarks PRIVATE QUXLANG_PARSER_BENCHMARK_CORPUS_DIR="${CMAKE_CURRENT_BINARY_DIR}/parser_benchmark_corpus")

add_executable(qxc sources/app/qxc.cpp)
target_link_libraries(qxc PRIVATE quxlang-loaders)


add_compile_definitions(BOOST_MPL_CFG_NO_PREPROCESSED_HEADER_GUARDS BOOST_MPL_LIMIT_LIST_SIZE=50)

get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)


# TOOD: This should be a flag instead of a variable




if (BUILD_TESTING)
    enable_testing()
    set(QUXLANG_TESTS_TESTDATA_PATH "${CMAKE_SOURCE_DIR}/tests/testdata")
    if (NOT EXISTS "${QUXLANG_TESTS_TESTDATA_PATH}")
        message(FATAL_ERROR "The specified test data path does not exist: ${QUXLANG_TESTS_TESTDATA_PATH}")
    endif ()
    add_executable(quxlang_gtests)
    add_executable(quxlang_long_gtests)
    add_executable(quxlang_math_gtests)
    add_executable(quxlang_static_gtests)
    target_sources(quxlang_gtests PRIVATE tests/gtests.cpp tests/expr_test_generator.cpp tests/querygraph_gtests.cpp)
    target_sources(quxlang_long_gtests PRIVATE tests/long_gtests.cpp)
    target_sources(quxlang_static_gtests PRIVATE tests/static_gtests.cpp)
    target_link_libraries(quxlang_gtests PUBLIC GTest::GTest GTest::Main quxlang quxlang-loaders)
    target_link_libraries(quxlang_long_gtests PRIVATE GTest::GTest GTest::Main quxlang)
    target_link_libraries(quxlang_static_gtests PUBLIC GTest::GTest GTest::Main quxlang quxlang-loaders)
    target_link_libraries(quxlang_math_gtests PRIVATE GTest::GTest GTest::Main quxlang)
    target_include_directories(quxlang_gtests PRIVATE sources/app)
    target_include_directories(quxlang_long_gtests PRIVATE include)
    target_include_directories(quxlang_math_gtests PRIVATE include)
    target_sources(quxlang_math_gtests PRIVATE tests/gtests_bytemath.cpp)
    target_compile_definitions(quxlang_gtests PRIVATE QUXLANG_TESTS_TESTDDATA_PATH="${QUXLANG_TESTS_TESTDATA_PATH}")
    target_compile_definitions(quxlang_static_gtests PRIVATE QUXLANG_TESTS_TESTDDATA_PATH="${QUXLANG_TESTS_TESTDATA_PATH}")
    gtest_discover_tests(quxlang_math_gtests)
    gtest_discover_tests(quxlang_gtests DISCOVERY_TIMEOUT 30)
    gtest_discover_tests(quxlang_static_gtests DISCOVERY_TIMEOUT 30)
endif ()

install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/quxlang"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
        TARGETS quxlang quxlang-loaders
        EXPORT quxlang_exports
)

install(
        TARGETS qxc
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

export(
        EXPORT quxlang_exports
        FILE "${CMAKE_CURRENT_BINARY_DIR}/QuxlangConfig.cmake"
        NAMESPACE Quxlang::
)

install(
        EXPORT quxlang_exports
        FILE QuxlangConfig.cmake
        NAMESPACE Quxlang::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Quxlang"
)
