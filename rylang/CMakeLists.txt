if (CMAKE_IS_TOP_LEVEL_PROJECT)
    project(RyLang)
endif ()

find_package(GTest REQUIRED)
find_package(Boost REQUIRED)
find_package(LLVM REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})

add_executable(rylang_main sources/main.cpp)
add_library(rylang include/rylang/data/canonical_pointer_type_reference.hpp include/rylang/ast/entity_ast.hpp include/rylang/ast/entity_ast_base.hpp sources/ast/entity_ast.cpp include/rylang/ast/variable_ast.hpp include/rylang/ast/functum_entity_ast.hpp include/rylang/data/entity_category.hpp include/rylang/fwd.hpp include/rylang/ast/class_entity_ast.hpp include/rylang/ast/namespace_entity_ast.hpp include/rylang/ast/variable_entity_ast.hpp sources/ast/class_entity_ast.cpp sources/ast/class_entity_ast.cpp include/rylang/ast/entity_ast_base.hpp include/rylang/data/input_source.hpp include/rylang/data/symbol_id.hpp include/rylang/data/lookup_type.hpp include/rylang/data/class_member_variable_list.hpp include/rylang/data/class_member_variable_declaration.hpp include/rylang/data/class_member_variable_precursor1.hpp include/rylang/data/lookup_chain.hpp include/rylang/data/lookup_singular.hpp include/rylang/data/class_id_ast_resolver.hpp include/rylang/ast/module_ast_precursor1.hpp include/rylang/data/global_name_ast_map.hpp sources/res/lookup_resolver.cpp include/rylang/ast/module_ast.hpp include/rylang/res/module_ast_precursor1_resolver.hpp sources/res/module_ast_precursor1_resolver.cpp include/rylang/res/files_in_module_resolver.hpp sources/res/files_in_module_resolver.cpp include/rylang/res/file_module_map_resolver.hpp include/rylang/data/file_module_map.hpp sources/res/file_module_map_resolver.cpp include/rylang/res/class_size_from_canonical_chain_resolver.hpp include/rylang/res/entity_ast_from_canonical_chain_resolver.hpp include/rylang/res/entity_ast_from_chain_resolver.hpp sources/res/entity_ast_from_cannonical_chain_resolver.cpp sources/res/entity_ast_from_chain_resolver.cpp include/rylang/res/module_ast_resolver.hpp sources/res/module_ast_resolver.cpp include/rylang/manipulators/merge_entity.hpp sources/merge_entity.cpp include/rylang/data/contextual_type_reference.hpp include/rylang/data/class_field_declaration.hpp include/rylang/data/canonical_type_reference.hpp include/rylang/data/canonical_lookup_chain.hpp sources/res/class_size_from_canonical_chain_resolver.cpp sources/res/class_field_list_from_canonical_chain_resolver.cpp include/rylang/res/class_field_list_from_canonical_chain_resolver.hpp include/rylang/res/canonical_symbol_from_contextual_symbol_resolver.hpp sources/res/canonical_type_ref_from_contextual_type_ref_resolver.cpp include/rylang/res/type_size_from_canonical_type_resolver.hpp sources/res/type_size_from_canonical_type_resolver.cpp include/rylang/res/machine_info_resolver.hpp include/rylang/data/machine_info.hpp include/rylang/ast/reference_type_ast.hpp include/rylang/ast/lookup_chain_ast.hpp include/rylang/data/absolute_lookup_reference.hpp include/rylang/data/proximate_lookup_reference.hpp include/rylang/data/class_layout.hpp include/rylang/data/class_field_info.hpp include/rylang/res/class_layout_from_canonical_chain_resolver.hpp sources/res/class_layout_from_canonical_chain_resolver.cpp include/rylang/res/type_placement_info_from_canonical_type_resolver.hpp include/rylang/data/type_placement_info.hpp sources/res/type_placement_info_from_canonical_type_resolver.cpp include/rylang/manipulators/struct_math.hpp include/rylang/res/entity_canonical_chain_exists_resolver.hpp sources/res/entity_canonical_chain_exists_resolver.cpp include/rylang/data/expression.hpp include/rylang/data/function_block.hpp include/rylang/data/function_statement.hpp include/rylang/data/function_if_statement.hpp include/rylang/data/function_expression_statement.hpp include/rylang/data/expression_add.hpp include/rylang/data/expression_call.hpp include/rylang/data/expression_lvalue_reference.hpp include/rylang/cow.hpp include/rylang/data/expression_multiply.hpp include/rylang/manipulators/expression_stringifier.hpp include/rylang/data/expression_copy_assign.hpp include/rylang/data/expression_move_assign.hpp include/rylang/data/expression_subtract.hpp include/rylang/data/expression_bool.hpp include/rylang/data/expression_numeric_literal.hpp include/rylang/data/expression_dotreference.hpp include/rylang/llvm_code_generator.hpp include/rylang/data/canonical_resolved_function_chain.hpp include/rylang/data/function_argument.hpp include/rylang/data/call_parameter_information.hpp include/rylang/res/function_overload_selection_resolver.hpp include/rylang/res/mangled_name_resolver.hpp include/rylang/res/canonical_type_is_implicitly_convertible_to_resolver.hpp include/rylang/res/overload_set_is_callable_with_resolver.hpp sources/res/canonical_type_is_implicitly_convertible_to_resolver.cpp sources/res/overload_set_is_callable_with_resolver.cpp sources/llvm_code_generator.cpp include/rylang/data/cpu_architecture.hpp include/rylang/data/llvm_proxy_types.hpp include/rylang/data/function_return_statement.hpp include/rylang/data/vm_allocate_storage.hpp include/rylang/data/vm_expr_load_address.hpp include/rylang/data/vm_block.hpp include/rylang/data/vm_executable_unit.hpp include/rylang/data/vm_procedure.hpp include/rylang/data/vm_procedure_interface.hpp include/rylang/data/vm_type.hpp include/rylang/data/vm_expression.hpp include/rylang/llvmg/vm_llvm_frame.hpp include/rylang/llvmg/vm_llvm_frame_item.hpp include/rylang/data/vm_expr_primitive_op.hpp include/rylang/manipulators/vm_type_alignment.hpp include/rylang/data/vm_expr_call.hpp include/rylang/data/vm_expr_dereference.hpp include/rylang/ordering.hpp include/rylang/data/qualified_symbol_reference.hpp include/rylang/manipulators/mangler.hpp sources/res/function_overload_selection_resolver.cpp include/rylang/res/function_qualified_reference_resolver.hpp sources/res/function_qualified_reference_resolver.cpp include/rylang/converters/qual_converters.hpp include/rylang/res/contextualized_reference_resolver.hpp sources/res/contextualized_reference_resolver.cpp include/rylang/manipulators/qmanip.hpp include/rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp
        sources/res/vm_procedure_from_canonical_functanoid_resolver.cpp
        include/rylang/variant_utils.hpp
        include/rylang/res/function_ast_resolver.hpp
        sources/res/function_ast_resolver.cpp
        include/rylang/manipulators/argmanip.hpp
        include/rylang/data/vm_generation_frameinfo.hpp
        include/rylang/data/vm_frame_variable.hpp
        include/rylang/manipulators/vmmanip.hpp
        include/rylang/data/vm_expr_store.hpp
        include/rylang/data/vm_if.hpp
        include/rylang/data/expression_equals.hpp
        include/rylang/data/numeric_literal.hpp
        include/rylang/data/function_frame_information.hpp
        sources/example_file.cpp
        include/rylang/res/operator_is_overloaded_with_resolver.hpp
        include/rylang/ast/class_entity_ast.hpp include/rylang/data/lookup_type.hpp
        sources/res/operator_is_overloaded_with_resolver.cpp
        include/rylang/res/symbol_canonical_chain_exists_resolver.hpp
        sources/res/symbol_canonical_chain_exists_resolver.cpp
        include/rylang/data/vm_expr_bound_value.hpp
        sources/manipulators/qmanip.cpp
        sources/manipulators/qualified_symbol_stringifier.cpp
        include/rylang/data/vm_expr_load_literal.hpp
        include/rylang/res/class_should_autogen_default_constructor_resolver.hpp
        sources/res/class_should_autogen_default_constructor_resolver.cpp
        include/rylang/data/vm_expr_access_field.hpp
        include/rylang/res/symbol_is_primitive_resolver.hpp
        include/rylang/res/symbol_is_class_resolver.hpp
        include/rylang/data/function_while_statement.hpp
        include/rylang/data/symbol_type.hpp
        include/rylang/res/functum_exists_and_is_callable_with_resolver.hpp
        sources/res/functum_exists_and_is_callable_with_resolver.cpp
        include/rylang/res/list_functum_overloads_resolver.hpp
        include/rylang/res/list_functum_overloads_resolver.hpp
        sources/res/list_functum_overloads_resolver.cpp
        include/rylang/operators.hpp
        include/rylang/data/vm_expr_reinterpret.hpp
        include/rylang/data/vm_expr_poison.hpp
        include/rylang/data/vm_expr_undef.hpp
        include/rylang/compiler.hpp
        include/rylang/res/functanoid_return_type_resolver.hpp
        sources/res/functanoid_return_type_resolver.cpp
        include/rylang/to_pretty_string.hpp
        include/rylang/data/function_delegate.hpp
        include/rylang/res/list_builtin_functum_overloads_resolver.hpp
        include/rylang/res/type_size_from_canonical_type_question.hpp
        sources/res/type_size_from_canonical_type_question.cpp
        include/rylang/res/type_placement_info_from_canonical_type_question.hpp
        sources/res/type_placement_info_from_canonical_type_question.cpp
        include/rylang/res/vm_procedure_from_canonical_functanoid_question.hpp
        include/rylang/res/detail/vm_procedure_builder.hpp
        include/rylang/res/called_functanoids_resolver.hpp
        sources/res/called_functanoids_resolver.cpp
        include/rylang/res/list_user_functum_overloads_resolver.hpp
        sources/res/list_user_functum_overloads_resolver.cpp
        sources/res/list_builtin_functum_overloads_resolver.cpp
        include/rylang/manipulators/tmanip.hpp
        include/rylang/res/call_params_of_function_ast_resolver.hpp
        sources/res/call_params_of_function_ast_resolver.cpp
        include/rylang/ast2/ast2_type_map.hpp
        include/rylang/ast2/ast2_named_variable_declaration.hpp
        include/rylang/res/function_overload_selection_resolver2.hpp
        include/rylang/data/functanoid_instanciation.hpp
        include/rylang/debug.hpp
        include/rylang/res/overload_set_instanciate_with_resolver.hpp
        sources/res/overload_set_instanciate_with_resolver.cpp
        include/rylang/ast/class_template_ast.hpp
        include/rylang/ast/class_templex_entity_ast.hpp
        include/rylang/ast2/ast2_entity.hpp
        include/rylang/ast2/ast2_class_declaration.hpp
        include/rylang/ast2/ast2_function_declaration.hpp
        include/rylang/ast2/ast2_class_template.hpp
        include/rylang/parsers/parse_class_body.hpp
        include/rylang/parsers/try_parse_class.hpp
        include/rylang/parsers/try_parse_class_template.hpp
        include/rylang/parsers/skip_whitespace.hpp
        include/rylang/parsers/skip_keyword_if_is.hpp
        include/rylang/parsers/iter_parse_keyword.hpp
        include/rylang/parsers/skip_symbol_if_is.hpp
        include/rylang/parsers/iter_parse_symbol.hpp
        include/rylang/parsers/try_parse_class_variable_declaration.hpp
        include/rylang/parsers/parse_comment.hpp
        include/rylang/parsers/iter_parse_line_comment.hpp
        include/rylang/parsers/parse_symbol.hpp
        include/rylang/parsers/parse_whitespace_and_comments.hpp
        include/rylang/parsers/ctype.hpp
        include/rylang/parsers/parse_whitespace.hpp
        include/rylang/parsers/try_parse_function_declaration.hpp
        include/rylang/parsers/try_parse_type_symbol.hpp
        include/rylang/parsers/try_parse_integral_keyword.hpp
        include/rylang/parsers/try_parse_class_function_declaration.hpp
        include/rylang/ast2/ast2_named_function_declaration.hpp
        include/rylang/parsers/parse_type_symbol.hpp
        include/rylang/parsers/parse_function_block.hpp
        include/rylang/parsers/parse_function_declaration.hpp
        include/rylang/parsers/parse_function_args.hpp
        include/rylang/ast2/ast2_function_arg.hpp
        include/rylang/parsers/try_parse_function_return_type.hpp
        include/rylang/parsers/try_parse_statement.hpp
        include/rylang/parsers/parse_if_statement.hpp
        include/rylang/parsers/parse_expression.hpp
        include/rylang/parsers/try_parse_expression.hpp
        include/rylang/parsers/parse_int.hpp
        include/rylang/parsers/parse_var_statement.hpp
        include/rylang/parsers/next_keyword.hpp
        include/rylang/parsers/parse_keyword.hpp
        include/rylang/parsers/try_parse_expression_statement.hpp
        include/rylang/parsers/parse_return_statement.hpp
        include/rylang/parsers/parse_while_statement.hpp
        include/rylang/parsers/iter_parse_number.hpp
        include/rylang/parsers/try_parse_function_callsite_expression.hpp
        include/rylang/parsers/parse_class.hpp
        include/rylang/ast2/ast2_namespace.hpp
        include/rylang/ast2/ast2manip.hpp
        include/rylang/res/type_map_resolver.hpp
        sources/res/type_map_resolver.cpp
        include/rylang/ast2/ast2_templex.hpp
        include/rylang/ast2/ast2_functum.hpp
        include/rylang/ast2/ast2_module.hpp

)

target_include_directories(rylang PUBLIC include)



target_sources(rylang PRIVATE sources/compiler.cpp

        include/rylang/compiler.hpp
        include/rylang/compiler_fwd.hpp
        sources/res/filelist_resolver.cpp
        include/rylang/compiler_fwd.hpp
        include/rylang/res/file_ast_resolver.hpp
        include/rylang/ast/member_variable_ast.hpp
        include/rylang/ast/file_ast.hpp
        include/rylang/res/filelist_resolver.hpp
        include/rylang/filelist.hpp
        include/rylang/res/file_content_resolver.hpp
        sources/res/file_content_resolver.cpp
        sources/res/file_ast_resolver.cpp
        include/rylang/collector.hpp include/rylang/ast/function_ast.hpp ../rpnx/include/rpnx/value.hpp include/rylang/ast/symbol_ref_ast.hpp include/rylang/parser.hpp include/rylang/ex/unexpected_eof.hpp include/rylang/ast/null_object_ast.hpp include/rylang/ast/null_object_ast.hpp
        include/rylang/ex/unexpected_eof.hpp include/rylang/data/class_list.hpp include/rylang/res/class_list_resolver.hpp sources/res/class_list_resolver.cpp sources/res/classes_per_file_resolver.cpp include/rylang/res/classes_per_file_resolver.hpp)
target_link_libraries(rylang PUBLIC graphlib Boost::boost)
target_link_libraries(rylang_main PUBLIC rylang)
target_precompile_headers(rylang PRIVATE
 [["rylang/compiler.hpp"]]
 <boost/variant.hpp>)


add_compile_definitions(-DBOOST_MPL_CFG_NO_PREPROCESSED_HEADER_GUARDS -DBOOST_MPL_LIMIT_LIST_SIZE=50)

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
    target_sources(rylang_gtests PRIVATE tests/gtests.cpp)
    target_link_libraries(rylang_gtests PRIVATE GTest::GTest GTest::Main rylang)
    gtest_discover_tests(rylang_gtests)
endif ()

