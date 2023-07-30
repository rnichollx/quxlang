if (CMAKE_IS_TOP_LEVEL_PROJECT)
   project(RyLang)
endif()

add_library(graphlib INTERFACE include/rpnx/graph_solver.hpp rylang/include/rylang/ast/class_entity_ast.hpp)

target_include_directories(graphlib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
add_executable(rylang rylang/include/rylang/ast/entity_ast.hpp rylang/include/rylang/ast/entity_ast_base.hpp rylang/sources/ast/entity_ast.cpp rylang/include/rylang/ast/variable_ast.hpp rylang/include/rylang/ast/function_entity_ast.hpp rylang/include/rylang/data/entity_category.hpp rylang/include/rylang/fwd.hpp rylang/include/rylang/ast/class_entity_ast.hpp rylang/include/rylang/ast/namespace_entity_ast.hpp rylang/include/rylang/ast/variable_entity_ast.hpp rylang/sources/ast/class_entity_ast.cpp rylang/sources/ast/class_entity_ast.cpp rylang/include/rylang/ast/entity_ast_base.hpp rylang/include/rylang/data/input_source.hpp)

target_sources(rylang PRIVATE rylang/sources/rylang_compiler.cpp
        rylang/sources/ast/type_ref_ast.cpp
        rylang/include/rylang/compiler.hpp
        rylang/include/rylang/compiler_fwd.hpp
        rylang/sources/res/filelist_resolver.cpp
        rylang/include/rylang/compiler_fwd.hpp rylang/include/rylang/res/file_ast_resolver.hpp rylang/include/rylang/ast/member_variable_ast.hpp rylang/include/rylang/ast/file_ast.hpp
        rylang/include/rylang/res/filelist_resolver.hpp rylang/include/rylang/filelist.hpp rylang/sources/main.cpp rylang/include/rylang/res/file_content_resolver.hpp rylang/sources/res/file_content_resolver.cpp rylang/sources/res/file_ast_resolver.cpp rylang/include/rylang/ast/class_ast.hpp rylang/include/rylang/collector.hpp rylang/include/rylang/ast/function_ast.hpp rylang/include/rylang/ast/type_ref_ast.hpp include/rpnx/value.hpp rylang/include/rylang/ast/symbol_ref_ast.hpp rylang/include/rylang/parser.hpp rylang/include/rylang/ex/unexpected_eof.hpp rylang/include/rylang/ast/null_object_ast.hpp rylang/include/rylang/ast/null_object_ast.hpp
        rylang/include/rylang/ex/unexpected_eof.hpp rylang/include/rylang/data/class_list.hpp rylang/include/rylang/res/class_list_resolver.hpp rylang/sources/res/class_list_resolver.cpp rylang/sources/res/classes_per_file_resolver.cpp rylang/include/rylang/res/classes_per_file_resolver.hpp rylang/include/rylang/ast/namespace_ast.hpp)
target_link_libraries(rylang PUBLIC graphlib)
target_include_directories(rylang PUBLIC rylang/include)

