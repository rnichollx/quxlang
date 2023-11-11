if (CMAKE_IS_TOP_LEVEL_PROJECT)
    project(RyLang)
endif ()

find_package(GTest REQUIRED)
find_package(Boost REQUIRED)
find_package(LLVM REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})

add_executable(rylang_main rylang/sources/main.cpp )
add_library(rylang rylang/include/rylang/data/canonical_pointer_type_reference.hpp rylang/include/rylang/ast/entity_ast.hpp rylang/include/rylang/ast/entity_ast_base.hpp rylang/sources/ast/entity_ast.cpp rylang/include/rylang/ast/variable_ast.hpp rylang/include/rylang/ast/functum_entity_ast.hpp rylang/include/rylang/data/entity_category.hpp rylang/include/rylang/fwd.hpp rylang/include/rylang/ast/class_entity_ast.hpp rylang/include/rylang/ast/namespace_entity_ast.hpp rylang/include/rylang/ast/variable_entity_ast.hpp rylang/sources/ast/class_entity_ast.cpp rylang/sources/ast/class_entity_ast.cpp rylang/include/rylang/ast/entity_ast_base.hpp rylang/include/rylang/data/input_source.hpp rylang/include/rylang/data/symbol_id.hpp rylang/include/rylang/data/lookup_type.hpp rylang/include/rylang/res/class_layout_resolver.hpp rylang/include/rylang/data/class_member_variable_list.hpp rylang/include/rylang/data/class_member_variable_declaration.hpp rylang/include/rylang/data/class_member_variable_precursor1.hpp rylang/include/rylang/data/lookup_chain.hpp rylang/include/rylang/data/lookup_singular.hpp rylang/sources/res/class_member_variable_list_resolver.cpp rylang/include/rylang/data/class_id_ast_resolver.hpp rylang/include/rylang/ast/module_ast_precursor1.hpp rylang/include/rylang/data/global_name_ast_map.hpp rylang/sources/res/lookup_resolver.cpp rylang/include/rylang/ast/module_ast.hpp rylang/include/rylang/res/module_ast_precursor1_resolver.hpp rylang/sources/res/module_ast_precursor1_resolver.cpp rylang/include/rylang/res/files_in_module_resolver.hpp rylang/sources/res/files_in_module_resolver.cpp rylang/include/rylang/res/file_module_map_resolver.hpp rylang/include/rylang/data/file_module_map.hpp rylang/sources/res/file_module_map_resolver.cpp rylang/include/rylang/res/class_size_from_canonical_chain_resolver.hpp rylang/include/rylang/res/entity_ast_from_canonical_chain_resolver.hpp rylang/include/rylang/res/entity_ast_from_chain_resolver.hpp rylang/sources/res/entity_ast_from_cannonical_chain_resolver.cpp rylang/sources/res/entity_ast_from_chain_resolver.cpp rylang/include/rylang/res/module_ast_resolver.hpp rylang/sources/res/module_ast_resolver.cpp rylang/include/rylang/manipulators/merge_entity.hpp rylang/sources/merge_entity.cpp rylang/include/rylang/data/contextual_type_reference.hpp rylang/include/rylang/data/class_field_declaration.hpp rylang/include/rylang/data/canonical_type_reference.hpp rylang/include/rylang/data/canonical_lookup_chain.hpp rylang/sources/res/class_size_from_canonical_chain_resolver.cpp rylang/sources/res/class_field_list_from_canonical_chain_resolver.cpp rylang/include/rylang/res/class_field_list_from_canonical_chain_resolver.hpp rylang/include/rylang/res/canonical_type_ref_from_contextual_type_ref_resolver.hpp rylang/sources/res/canonical_type_ref_from_contextual_type_ref_resolver.cpp rylang/include/rylang/res/type_size_from_canonical_type_resolver.hpp rylang/sources/res/type_size_from_canonical_type_resolver.cpp rylang/include/rylang/res/machine_info_resolver.hpp rylang/include/rylang/data/machine_info.hpp rylang/include/rylang/ast/reference_type_ast.hpp rylang/include/rylang/ast/lookup_chain_ast.hpp rylang/include/rylang/data/absolute_lookup_reference.hpp rylang/include/rylang/data/proximate_lookup_reference.hpp rylang/include/rylang/data/class_layout.hpp rylang/include/rylang/data/class_field_info.hpp rylang/include/rylang/res/class_layout_from_canonical_chain_resolver.hpp rylang/sources/res/class_layout_from_canonical_chain_resolver.cpp rylang/include/rylang/res/type_placement_info_from_canonical_type_resolver.hpp rylang/include/rylang/data/type_placement_info.hpp rylang/sources/res/type_placement_info_from_canonical_type_resolver.cpp rylang/include/rylang/manipulators/struct_math.hpp rylang/include/rylang/res/entity_canonical_chain_exists_resolver.hpp rylang/sources/res/entity_canonical_chain_exists_resolver.cpp rylang/include/rylang/data/expression.hpp rylang/include/rylang/data/function_block.hpp rylang/include/rylang/data/function_statement.hpp rylang/include/rylang/data/function_if_statement.hpp rylang/include/rylang/data/function_expression_statement.hpp rylang/include/rylang/data/expression_add.hpp rylang/include/rylang/data/expression_call.hpp rylang/include/rylang/data/expression_lvalue_reference.hpp rylang/include/rylang/cow.hpp rylang/include/rylang/data/s_expression.hpp rylang/include/rylang/data/s_expression_add.hpp rylang/include/rylang/data/s_expression_call.hpp rylang/include/rylang/data/expression_multiply.hpp rylang/include/rylang/manipulators/expression_stringifier.hpp rylang/include/rylang/data/expression_copy_assign.hpp rylang/include/rylang/data/expression_move_assign.hpp rylang/include/rylang/data/expression_subtract.hpp rylang/include/rylang/data/expression_bool.hpp rylang/include/rylang/data/expression_numeric_literal.hpp rylang/include/rylang/data/expression_dotreference.hpp rylang/include/rylang/llvm_code_generator.hpp rylang/include/rylang/data/canonical_resolved_function_chain.hpp rylang/include/rylang/data/function_argument.hpp rylang/include/rylang/data/call_overload_set.hpp rylang/include/rylang/res/function_overload_selection_resolver.hpp rylang/include/rylang/res/mangled_name_resolver.hpp rylang/include/rylang/res/canonical_type_is_implicitly_convertible_to_resolver.hpp rylang/include/rylang/res/overload_set_is_callable_with_resolver.hpp rylang/sources/res/canonical_type_is_implicitly_convertible_to_resolver.cpp rylang/sources/res/overload_set_is_callable_with_resolver.cpp rylang/sources/llvm_code_generator.cpp rylang/include/rylang/data/cpu_architecture.hpp rylang/include/rylang/data/llvm_proxy_types.hpp rylang/include/rylang/data/codepath_execution_block.hpp rylang/include/rylang/data/codepath_execution_return.hpp rylang/include/rylang/data/codepath_expression.hpp rylang/include/rylang/data/function_return_statement.hpp rylang/include/rylang/data/vm_allocate_storage.hpp rylang/include/rylang/data/vm_expr_load_address.hpp rylang/include/rylang/data/vm_block.hpp rylang/include/rylang/data/vm_executable_unit.hpp rylang/include/rylang/data/vm_procedure.hpp rylang/include/rylang/data/vm_procedure_interface.hpp rylang/include/rylang/data/vm_type.hpp rylang/include/rylang/data/vm_expression.hpp rylang/include/rylang/llvmg/vm_llvm_frame.hpp rylang/include/rylang/llvmg/vm_llvm_frame_item.hpp rylang/include/rylang/data/vm_expr_primitive_op.hpp rylang/include/rylang/manipulators/vm_type_alignment.hpp rylang/include/rylang/data/vm_expr_call.hpp rylang/include/rylang/data/vm_expr_dereference.hpp rylang/include/rylang/ordering.hpp rylang/include/rylang/data/qualified_reference.hpp rylang/include/rylang/manipulators/mangler.hpp rylang/sources/res/function_overload_selection_resolver.cpp rylang/include/rylang/res/function_qualified_reference_resolver.hpp rylang/sources/res/function_qualified_reference_resolver.cpp rylang/include/rylang/converters/qual_converters.hpp rylang/include/rylang/res/contextualized_reference_resolver.hpp rylang/sources/res/contextualized_reference_resolver.cpp rylang/include/rylang/manipulators/qmanip.hpp rylang/include/rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp
        rylang/sources/res/vm_procedure_from_canonical_functanoid_resolver.cpp
        rylang/include/rylang/variant_utils.hpp
        rylang/include/rylang/res/function_ast_resolver.hpp
        rylang/sources/res/function_ast_resolver.cpp
        rylang/include/rylang/manipulators/argmanip.hpp
        rylang/include/rylang/data/vm_generation_frameinfo.hpp
        rylang/include/rylang/data/vm_frame_variable.hpp
        rylang/include/rylang/manipulators/vmmanip.hpp
        rylang/include/rylang/data/vm_expr_store.hpp
        rylang/include/rylang/data/vm_if.hpp
        rylang/include/rylang/data/expression_equals.hpp
        rylang/include/rylang/data/numeric_literal.hpp
        rylang/include/rylang/res/function_frame_information_resolver.hpp
        rylang/include/rylang/data/function_frame_information.hpp
        rylang/sources/res/function_frame_information_resolver.cpp
        rylang/sources/example_file.cpp)

target_include_directories(rylang PUBLIC rylang/include)
add_library(graphlib INTERFACE include/rpnx/resolver_utilities.hpp rylang/include/rylang/ast/class_entity_ast.hpp rylang/include/rylang/data/lookup_type.hpp)

target_include_directories(graphlib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(rylang PRIVATE rylang/sources/compiler.cpp

        rylang/include/rylang/compiler.hpp
        rylang/include/rylang/compiler_fwd.hpp
        rylang/sources/res/filelist_resolver.cpp
        rylang/include/rylang/compiler_fwd.hpp
        rylang/include/rylang/res/file_ast_resolver.hpp
        rylang/include/rylang/ast/member_variable_ast.hpp
        rylang/include/rylang/ast/file_ast.hpp
        rylang/include/rylang/res/filelist_resolver.hpp
        rylang/include/rylang/filelist.hpp
        rylang/include/rylang/res/file_content_resolver.hpp
        rylang/sources/res/file_content_resolver.cpp
        rylang/sources/res/file_ast_resolver.cpp
        rylang/include/rylang/collector.hpp rylang/include/rylang/ast/function_ast.hpp include/rpnx/value.hpp rylang/include/rylang/ast/symbol_ref_ast.hpp rylang/include/rylang/parser.hpp rylang/include/rylang/ex/unexpected_eof.hpp rylang/include/rylang/ast/null_object_ast.hpp rylang/include/rylang/ast/null_object_ast.hpp
        rylang/include/rylang/ex/unexpected_eof.hpp rylang/include/rylang/data/class_list.hpp rylang/include/rylang/res/class_list_resolver.hpp rylang/sources/res/class_list_resolver.cpp rylang/sources/res/classes_per_file_resolver.cpp rylang/include/rylang/res/classes_per_file_resolver.hpp)
target_link_libraries(rylang PUBLIC graphlib Boost::boost)
target_link_libraries(rylang_main PUBLIC rylang)


add_compile_definitions(-DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS -DBOOST_MPL_LIMIT_LIST_SIZE=50)

get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)


# for each property that starts with LLVM, link to it
foreach (target IN LISTS targets)
    if ((target MATCHES "^LLVM.*") AND NOT target MATCHES "^LLVMLineEditor$")
        message(STATUS "Linking to ${target}")
        target_link_libraries(rylang PUBLIC ${target})
    else ()
        message(STATUS "Not linking to ${target}")
    endif ()
endforeach ()

if (GTest_FOUND)
    enable_testing()
    add_executable(rylang_gtests)
    target_sources(rylang_gtests PRIVATE rylang/tests/gtests.cpp)
    target_link_libraries(rylang_gtests PRIVATE GTest::GTest GTest::Main rylang)
    gtest_discover_tests(rylang_gtests)
endif ()

