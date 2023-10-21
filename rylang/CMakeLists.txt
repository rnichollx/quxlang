if (CMAKE_IS_TOP_LEVEL_PROJECT)
    project(RyLang)
endif ()

find_package(GTest REQUIRED)
find_package(Boost REQUIRED)

add_executable(rylang_main rylang/sources/main.cpp
        rylang/include/rylang/data/canonical_pointer_type_reference.hpp)
add_library(rylang rylang/include/rylang/ast/entity_ast.hpp rylang/include/rylang/ast/entity_ast_base.hpp rylang/sources/ast/entity_ast.cpp rylang/include/rylang/ast/variable_ast.hpp rylang/include/rylang/ast/function_entity_ast.hpp rylang/include/rylang/data/entity_category.hpp rylang/include/rylang/fwd.hpp rylang/include/rylang/ast/class_entity_ast.hpp rylang/include/rylang/ast/namespace_entity_ast.hpp rylang/include/rylang/ast/variable_entity_ast.hpp rylang/sources/ast/class_entity_ast.cpp rylang/sources/ast/class_entity_ast.cpp rylang/include/rylang/ast/entity_ast_base.hpp rylang/include/rylang/data/input_source.hpp rylang/include/rylang/data/symbol_id.hpp rylang/include/rylang/data/lookup_type.hpp rylang/include/rylang/res/class_layout_resolver.hpp rylang/include/rylang/res/class_member_variable_list_resolver.hpp rylang/include/rylang/data/class_member_variable_list.hpp rylang/include/rylang/data/class_member_variable_declaration.hpp rylang/include/rylang/data/class_member_variable_precursor1.hpp rylang/include/rylang/data/lookup_chain.hpp rylang/include/rylang/data/lookup_singular.hpp rylang/sources/res/class_member_variable_list_resolver.cpp rylang/include/rylang/data/class_id_ast_resolver.hpp rylang/include/rylang/ast/module_ast_precursor1.hpp rylang/include/rylang/data/global_name_ast_map.hpp rylang/sources/res/lookup_resolver.cpp rylang/include/rylang/ast/module_ast.hpp
        rylang/include/rylang/res/module_ast_precursor1_resolver.hpp
        rylang/sources/res/module_ast_precursor1_resolver.cpp
        rylang/include/rylang/res/files_in_module_resolver.hpp
        rylang/sources/res/files_in_module_resolver.cpp
        rylang/include/rylang/res/file_module_map_resolver.hpp
        rylang/include/rylang/data/file_module_map.hpp
        rylang/sources/res/file_module_map_resolver.cpp
        rylang/include/rylang/res/class_size_from_canonical_chain_resolver.hpp
        rylang/include/rylang/res/canonical_chain_resolver.hpp
        rylang/sources/res/canonical_chain_resolver.cpp
        rylang/include/rylang/res/entity_ast_from_canonical_chain_resolver.hpp
        rylang/include/rylang/res/entity_ast_from_chain_resolver.hpp
        rylang/sources/res/entity_ast_from_cannonical_chain_resolver.cpp
        rylang/sources/res/entity_ast_from_chain_resolver.cpp
        rylang/include/rylang/res/module_ast_resolver.hpp
        rylang/sources/res/module_ast_resolver.cpp
        rylang/include/rylang/res/type_ref_sa_resolver.hpp
        rylang/include/rylang/manipulators/merge_entity.hpp
        rylang/include/rylang/manipulators/merge_entity.hpp
        rylang/sources/merge_entity.cpp
        rylang/include/rylang/data/contextual_type_reference.hpp
        rylang/include/rylang/data/type_reference.hpp
        rylang/include/rylang/data/pointer_reference.hpp
        rylang/include/rylang/data/class_field.hpp
        rylang/include/rylang/data/canonical_type_reference.hpp
        rylang/include/rylang/data/canonical_lookup_chain.hpp
        rylang/sources/res/class_size_from_canonical_chain_resolver.cpp
        rylang/sources/res/class_field_list_from_canonical_chain_resolver.cpp
        rylang/include/rylang/res/class_field_list_from_canonical_chain_resolver.hpp
        rylang/include/rylang/res/canonical_type_ref_from_contextual_type_ref_resolver.hpp
        rylang/sources/res/canonical_type_ref_from_contextual_type_ref_resolver.cpp
        rylang/include/rylang/res/type_size_from_canonical_type_resolver.hpp
        rylang/sources/res/type_size_from_canonical_type_resolver.cpp
        rylang/include/rylang/res/machine_info_resolver.hpp
        rylang/include/rylang/data/machine_info.hpp)

target_include_directories(rylang PUBLIC rylang/include)
add_library(graphlib INTERFACE include/rpnx/resolver_utilities.hpp rylang/include/rylang/ast/class_entity_ast.hpp rylang/include/rylang/data/lookup_type.hpp)

target_include_directories(graphlib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(rylang PRIVATE rylang/sources/rylang_compiler.cpp
        rylang/sources/ast/type_ref_ast.cpp
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
        rylang/include/rylang/ast/class_ast.hpp
         rylang/include/rylang/collector.hpp rylang/include/rylang/ast/function_ast.hpp rylang/include/rylang/ast/type_ref_ast.hpp include/rpnx/value.hpp rylang/include/rylang/ast/symbol_ref_ast.hpp rylang/include/rylang/parser.hpp rylang/include/rylang/ex/unexpected_eof.hpp rylang/include/rylang/ast/null_object_ast.hpp rylang/include/rylang/ast/null_object_ast.hpp
        rylang/include/rylang/ex/unexpected_eof.hpp rylang/include/rylang/data/class_list.hpp rylang/include/rylang/res/class_list_resolver.hpp rylang/sources/res/class_list_resolver.cpp rylang/sources/res/classes_per_file_resolver.cpp rylang/include/rylang/res/classes_per_file_resolver.hpp rylang/include/rylang/ast/namespace_ast.hpp)
target_link_libraries(rylang PUBLIC graphlib Boost::boost)
target_link_libraries(rylang_main PUBLIC rylang)

if (GTest_FOUND)
    enable_testing()
    add_executable(rylang_gtests)
    target_sources(rylang_gtests PRIVATE rylang/tests/gtests.cpp)
    target_link_libraries(rylang_gtests PRIVATE GTest::GTest GTest::Main rylang)
    gtest_discover_tests(rylang_gtests)
endif ()

