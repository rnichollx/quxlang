if (CMAKE_IS_TOP_LEVEL_PROJECT)
    project(RyLang)
endif ()

find_package(GTest REQUIRED)
find_package(Boost REQUIRED)
find_package(LLVM REQUIRED)
#find_package(lld REQUIRED)
find_package(yaml-cpp REQUIRED)
#target_link_libraries(quxlang PRIVATE yaml-cpp::yaml-cpp)
include_directories(${LLVM_INCLUDE_DIRS})

add_executable(quxlang_main sources/main.cpp)
add_library(quxlang include/quxlang/data/canonical_pointer_type_reference.hpp include/quxlang/data/entity_category.hpp include/quxlang/fwd.hpp include/quxlang/data/input_source.hpp include/quxlang/data/symbol_id.hpp include/quxlang/data/lookup_type.hpp include/quxlang/data/class_member_variable_list.hpp include/quxlang/data/class_member_variable_declaration.hpp include/quxlang/data/class_member_variable_precursor1.hpp include/quxlang/data/lookup_chain.hpp include/quxlang/data/lookup_singular.hpp include/quxlang/data/class_id_ast_resolver.hpp include/quxlang/data/global_name_ast_map.hpp include/quxlang/res/module_ast_precursor1_resolver.hpp include/quxlang/res/files_in_module_resolver.hpp sources/res/files_in_module_resolver.cpp include/quxlang/res/file_module_map_resolver.hpp include/quxlang/data/file_module_map.hpp include/quxlang/res/class_size_from_canonical_chain_resolver.hpp include/quxlang/res/entity_ast_from_canonical_chain_resolver.hpp include/quxlang/res/entity_ast_from_chain_resolver.hpp sources/res/entity_ast_from_chain_resolver.cpp include/quxlang/res/module_ast_resolver.hpp sources/res/module_ast_resolver.cpp include/quxlang/manipulators/merge_entity.hpp sources/merge_entity.cpp include/quxlang/data/contextual_type_reference.hpp include/quxlang/data/class_field_declaration.hpp include/quxlang/data/canonical_type_reference.hpp include/quxlang/data/canonical_lookup_chain.hpp sources/res/class_size_from_canonical_chain_resolver.cpp sources/res/class_field_list.cpp include/quxlang/res/class_field_list_resolver.hpp include/quxlang/res/canonical_symbol_from_contextual_symbol_resolver.hpp sources/res/canonical_type_ref_from_contextual_type_ref_resolver.cpp include/quxlang/res/type_size_from_canonical_type_resolver.hpp sources/res/type_size_from_canonical_type_resolver.cpp include/quxlang/res/machine_info_resolver.hpp include/quxlang/data/absolute_lookup_reference.hpp include/quxlang/data/proximate_lookup_reference.hpp include/quxlang/data/class_layout.hpp include/quxlang/data/class_field_info.hpp include/quxlang/res/class_layout_resolver.hpp sources/res/class_layout.cpp include/quxlang/res/type_placement_info_from_canonical_type_resolver.hpp include/quxlang/data/type_placement_info.hpp sources/res/type_placement_info_from_canonical_type_resolver.cpp include/quxlang/manipulators/struct_math.hpp include/quxlang/res/entity_canonical_chain_exists_resolver.hpp sources/res/entity_canonical_chain_exists_resolver.cpp include/quxlang/data/expression.hpp include/quxlang/data/function_block.hpp include/quxlang/data/function_statement.hpp include/quxlang/data/function_if_statement.hpp include/quxlang/data/function_expression_statement.hpp include/quxlang/data/expression_add.hpp include/quxlang/data/expression_call.hpp include/quxlang/data/expression_lvalue_reference.hpp include/quxlang/cow.hpp include/quxlang/data/expression_multiply.hpp include/quxlang/manipulators/expression_stringifier.hpp include/quxlang/data/expression_copy_assign.hpp include/quxlang/data/expression_move_assign.hpp include/quxlang/data/expression_subtract.hpp include/quxlang/data/expression_bool.hpp include/quxlang/data/expression_numeric_literal.hpp include/quxlang/data/expression_dotreference.hpp include/quxlang/backends/llvm/llvm_code_generator.hpp include/quxlang/data/canonical_resolved_function_chain.hpp include/quxlang/data/function_argument.hpp include/quxlang/data/call_parameter_information.hpp include/quxlang/res/function_instanciation.hpp include/quxlang/res/mangled_name_resolver.hpp include/quxlang/res/implicitly_convertible_to.hpp include/quxlang/res/overload_set_is_callable_with_resolver.hpp sources/res/implicitly_convertible_to.cpp sources/res/overload_set_is_callable_with_resolver.cpp sources/backends/llvm/llvm_code_generator.cpp include/quxlang/data/cpu_architecture.hpp include/quxlang/data/llvm_proxy_types.hpp include/quxlang/data/function_return_statement.hpp include/quxlang/data/vm_allocate_storage.hpp include/quxlang/data/vm_expr_load_reference.hpp include/quxlang/data/vm_block.hpp include/quxlang/data/vm_executable_unit.hpp include/quxlang/data/vm_procedure.hpp include/quxlang/data/vm_procedure_interface.hpp include/quxlang/data/vm_type.hpp include/quxlang/data/vm_expression.hpp include/quxlang/backends/llvm/vm_llvm_frame.hpp include/quxlang/backends/llvm/vm_llvm_frame_item.hpp include/quxlang/data/vm_expr_primitive_op.hpp include/quxlang/manipulators/vm_type_alignment.hpp include/quxlang/data/vm_invoke.hpp include/quxlang/data/vm_expr_dereference.hpp include/quxlang/ordering.hpp include/quxlang/data/type_symbol.hpp include/quxlang/manipulators/mangler.hpp include/quxlang/res/function_qualified_reference_resolver.hpp sources/res/function_qualified_reference_resolver.cpp include/quxlang/converters/qual_converters.hpp include/quxlang/res/contextualized_reference_resolver.hpp sources/res/contextualized_reference_resolver.cpp include/quxlang/manipulators/qmanip.hpp include/quxlang/res/vm_procedure_from_canonical_functanoid_resolver.hpp
        sources/res/vm_procedure_from_canonical_functanoid_resolver.cpp
        include/quxlang/variant_utils.hpp
        sources/res/function_declaration.cpp
        include/quxlang/manipulators/argmanip.hpp
        include/quxlang/data/vm_generation_frameinfo.hpp
        include/quxlang/data/vm_frame_variable.hpp
        include/quxlang/manipulators/vmmanip.hpp
        include/quxlang/data/vm_expr_store.hpp
        include/quxlang/data/vm_if.hpp
        include/quxlang/data/expression_equals.hpp
        include/quxlang/data/numeric_literal.hpp
        include/quxlang/data/function_frame_information.hpp
        sources/example_file.cpp
        include/quxlang/res/operator_is_overloaded_with_resolver.hpp
        include/quxlang/data/lookup_type.hpp
        sources/res/operator_is_overloaded_with_resolver.cpp
        include/quxlang/res/symbol_canonical_chain_exists_resolver.hpp
        sources/res/symbol_canonical_chain_exists_resolver.cpp
        include/quxlang/data/vm_expr_bound_value.hpp
        sources/manipulators/qmanip.cpp
        sources/manipulators/qualified_symbol_stringifier.cpp
        include/quxlang/data/vm_expr_load_literal.hpp
        include/quxlang/res/class_should_autogen_default_constructor_resolver.hpp
        sources/res/class_should_autogen_default_constructor_resolver.cpp
        include/quxlang/data/vm_expr_access_field.hpp
        include/quxlang/res/symbol_is_primitive_resolver.hpp
        include/quxlang/res/symbol_is_class_resolver.hpp
        include/quxlang/data/function_while_statement.hpp
        include/quxlang/data/symbol_type.hpp
        include/quxlang/res/functum_exists_and_is_callable_with_resolver.hpp
        include/quxlang/res/list_functum_overloads_resolver.hpp
        include/quxlang/res/list_functum_overloads_resolver.hpp
        sources/res/list_functum_overloads_resolver.cpp
        include/quxlang/operators.hpp
        include/quxlang/data/vm_expr_reinterpret.hpp
        include/quxlang/data/vm_expr_poison.hpp
        include/quxlang/data/vm_expr_undef.hpp
        include/quxlang/compiler.hpp
        include/quxlang/res/functanoid_return_type_resolver.hpp
        sources/res/functanoid_return_type_resolver.cpp
        include/quxlang/to_pretty_string.hpp
        include/quxlang/data/function_delegate.hpp
        include/quxlang/res/list_builtin_functum_overloads_resolver.hpp
        include/quxlang/res/type_size_from_canonical_type_question.hpp
        sources/res/type_size_from_canonical_type_question.cpp
        include/quxlang/res/type_placement_info_from_canonical_type_question.hpp
        sources/res/type_placement_info_from_canonical_type_question.cpp
        include/quxlang/res/vm_procedure_from_canonical_functanoid_question.hpp
        include/quxlang/res/detail/vm_procedure_builder.hpp
        include/quxlang/res/called_functanoids_resolver.hpp
        sources/res/called_functanoids_resolver.cpp
        include/quxlang/res/list_user_functum_overloads_resolver.hpp
        sources/res/list_user_functum_overloads_resolver.cpp
        sources/res/list_builtin_functum_overloads_resolver.cpp
        include/quxlang/manipulators/tmanip.hpp
        include/quxlang/res/call_params_of_function_ast_resolver.hpp
        include/quxlang/ast2/ast2_type_map.hpp
        include/quxlang/data/functanoid_instanciation.hpp
        ../rpnx/include/rpnx/debug.hpp
        include/quxlang/res/overload_set_instanciate_with_resolver.hpp
        sources/res/overload_set_instanciate_with_resolver.cpp


        include/quxlang/ast2/ast2_entity.hpp
        include/quxlang/ast2/ast2_function_delegate.hpp
        include/quxlang/parsers/parse_class_body.hpp
        include/quxlang/parsers/try_parse_class.hpp
        include/quxlang/parsers/try_parse_class_template.hpp
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
        include/quxlang/res/template_instanciation_parameter_set_resolver.hpp
        include/quxlang/data/temploid_instanciation_parameter_set.hpp
        sources/res/template_instanciation_parameter_set_resolver.cpp
        sources/res/template_instanciation_ast_resolver.cpp
        include/quxlang/res/template_instanciation_ast_resolver.hpp
        include/quxlang/res/functanoid_parameter_map.hpp
        include/quxlang/res/temploid_instanciation_parameter_set_resolver.hpp
        sources/res/temploid_instanciation_parameter_set_resolver.cpp
        sources/res/functanoid_parameter_map.cpp
        include/quxlang/ast2/ast2_externfunc.hpp
        include/quxlang/data/output_object_symbol.hpp
        include/quxlang/data/code_relocation.hpp
        include/quxlang/macros.hpp
        include/quxlang/manipulators/symbolmap.hpp
        include/quxlang/manipulators/llvm_symbol_relocation.hpp
        sources/manipulators/llvm_symbol_relocation.cpp
        include/quxlang/manipulators/convert_llvm_object.hpp
        sources/manipulators/convert_llvm_object.cpp
        include/quxlang/data/machine.hpp
        include/quxlang/manipulators/llvm_lookup.hpp
        include/quxlang/input_aggregator.hpp
        sources/input_aggregator.cpp
        include/quxlang/vmir/interpreter.hpp
        include/quxlang/linker/elf_linker.hpp
        include/quxlang/parsers/asm/arm_assembler.hpp
        include/quxlang/asm/asm.hpp
        include/quxlang/parsers/parse_asm_procedure.hpp
        include/quxlang/backends/llvm/qux_mc_streamer.hpp
        include/quxlang/backends/asm/arm_asm_converter.hpp
        sources/res/symbol_type.cpp
        sources/res/asm_procedure_from_symbol_resolver.cpp
        include/quxlang/parsers/asm/asm_callable.hpp
        include/quxlang/parsers/asm/parse_register.hpp
        include/quxlang/backends/elf/elf.hpp
        include/quxlang/parsers/import.hpp
        include/quxlang/parsers/extern.hpp
        include/quxlang/parsers/string_literal.hpp
        include/quxlang/res/extern_linksymbol_resolver.hpp
        sources/res/extern_linksymbol_resolver.cpp
        include/quxlang/parsers/symbol.hpp
        include/quxlang/parsers/procedure_ref.hpp
        include/quxlang/res/procedure_linksymbol_resolver.hpp
        include/quxlang/res/temploid_ast_resolver.hpp
        sources/res/procedure_linksymbol_resolver.cpp
        include/quxlang/parsers/linkname.hpp
        include/quxlang/questions/questions.hpp
        include/quxlang/data/target_configuration.hpp
        include/quxlang/res/module_sources_resolver.hpp
        sources/res/module_sources_resolver.cpp
        include/quxlang/res/ast2_module_resolver.hpp
        include/quxlang/res/module_source_name_resolver.hpp
        sources/res/module_source_name_resolver.cpp
        include/quxlang/data/interp_value.hpp
        sources/interpreter.cpp
        include/quxlang/res/interpret_value_resolver.hpp
        sources/res/interpret_value_resolver.cpp
        include/quxlang/res/interpret_bool_resolver.hpp
        sources/res/interpret_bool_resolver.cpp
        include/quxlang/parsers/declaration.hpp
        include/quxlang/data/variants.hpp
        include/quxlang/parsers/include_if.hpp
        include/quxlang/parsers/expression/target.hpp
        include/quxlang/data/builtins.hpp
        include/quxlang/res/expr/co_vmir_expression_emitter.hpp
        sources/co_vmir_emitter.cpp
        sources/res/functum_selection.cpp
        include/quxlang/res/function_declaration.hpp
        include/quxlang/data/function_header.hpp
        include/quxlang/res/functum_instanciation.hpp
        include/quxlang/res/functum_select_function.hpp
        include/quxlang/data/fwd.hpp
        include/quxlang/res/function_positional_parameter_names_resolver.hpp
        sources/res/function_positional_parameter_names_resolver.cpp
        include/quxlang/res/declaroids_resolver.hpp
        include/quxlang/res/resolver.hpp
        sources/res/declaroids_resolver.cpp
        include/quxlang/res/symboid_resolver.hpp
        include/quxlang/res/symboid_declaroids_resolver.hpp
        include/quxlang/res/symboid_subdeclaroids.hpp
        sources/res/symboid_sub_declaroids.cpp
        include/quxlang/res/instanciation.hpp
        sources/res/symboid_resolver.cpp
        sources/res/temploid_instanciation_resolver.cpp
        sources/res/functum_instanciation.cpp
        include/quxlang/res/symbol_type.hpp
        include/quxlang/res/target.hpp
        include/quxlang/res/template_instanciation.hpp
        include/quxlang/res/templex_select_template.hpp
        include/quxlang/res/resolvers.hpp
        sources/res/function_instanciation.cpp
        sources/res/functum_exists_and_is_callable_with.cpp
        include/quxlang/builtins.hpp
        include/quxlang/data/builtin_functions.hpp
        include/quxlang/res/function_builtin.hpp
        sources/res/function_builtin.cpp
        include/quxlang/res/vm_procedure2.hpp
        sources/res/vm_procedure2.cpp
        sources/res/template_instanciation_resolver.cpp
        include/quxlang/vmir2/assembly.hpp
        sources/vmir2/assembler.cpp
        ../rpnx/include/rpnx/simple_coroutine.hpp
        include/quxlang/compiler_binding.hpp
        include/quxlang/res/expr_ir2.hpp
        sources/res/expr_ir2.cpp
        include/quxlang/res/lookup.hpp
        sources/res/lookup.cpp
        sources/res/variable_type_resolver.cpp

)

add_executable(qxc sources/app/qxc.cpp)

add_executable(aprinter_test tests/aprinter_test.cpp
        include/quxlang/res/variable.hpp)

add_library(quxlang-loaders
        sources/loaders/module_loader.cpp
        sources/loaders/source_loader.cpp

        include/quxlang/module_loader.hpp
        include/quxlang/source_loader.hpp)
target_link_libraries(quxlang-loaders PUBLIC quxlang yaml-cpp::yaml-cpp)

target_link_libraries(qxc PUBLIC quxlang quxlang-loaders yaml-cpp::yaml-cpp)

target_link_libraries(aprinter_test PUBLIC quxlang quxlang-loaders yaml-cpp::yaml-cpp)

target_include_directories(quxlang PUBLIC include)


target_sources(quxlang PRIVATE sources/compiler.cpp

        include/quxlang/compiler.hpp
        include/quxlang/compiler_fwd.hpp
        sources/res/filelist_resolver.cpp
        include/quxlang/compiler_fwd.hpp
        include/quxlang/res/file_ast_resolver.hpp


        include/quxlang/res/filelist_resolver.hpp
        include/quxlang/filelist.hpp
        include/quxlang/res/file_content_resolver.hpp
        sources/res/file_content_resolver.cpp
        ../rpnx/include/rpnx/value.hpp include/quxlang/ex/unexpected_eof.hpp
        include/quxlang/ex/unexpected_eof.hpp include/quxlang/data/class_list.hpp
        include/quxlang/data/target_configuration.hpp)
target_link_libraries(quxlang PUBLIC graphlib Boost::boost Boost::headers)
target_link_libraries(quxlang_main PUBLIC quxlang)
#target_precompile_headers(quxlang PRIVATE
#        [["quxlang/compiler.hpp"]]
#        <boost/variant.hpp>)


add_compile_definitions(BOOST_MPL_CFG_NO_PREPROCESSED_HEADER_GUARDS BOOST_MPL_LIMIT_LIST_SIZE=50)

get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)


# for each property that starts with LLVM, link to it
foreach (target IN LISTS targets)
    if ((target MATCHES "^LLVM.*" OR target MATCHES "^lld.*") AND NOT target MATCHES "^LLVMLineEditor$")
        message(STATUS "Linking to ${target}")
        target_link_libraries(quxlang PUBLIC ${target})
    else ()
        message(STATUS "Not linking to ${target}")
    endif ()
endforeach ()

# TOOD: This should be a flag instead of a variable


set(QUXLANG_TESTS_TESTDATA_PATH "${CMAKE_SOURCE_DIR}/quxlang/tests/testdata" CACHE PATH "Path to the test data directory")

if (GTest_FOUND)
    enable_testing()
    add_executable(quxlang_gtests)
    target_sources(quxlang_gtests PRIVATE tests/gtests.cpp tests/expr_test_generator.cpp)
    target_link_libraries(quxlang_gtests PRIVATE GTest::GTest GTest::Main quxlang quxlang-loaders)
    #escape the path



    target_compile_definitions(quxlang_gtests PRIVATE QUXLANG_TESTS_TESTDDATA_PATH="${QUXLANG_TESTS_TESTDATA_PATH}")
    gtest_discover_tests(quxlang_gtests)
endif ()

