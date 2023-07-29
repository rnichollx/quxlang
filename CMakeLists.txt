cmake_minimum_required(VERSION 3.22)
project(ryanscript1031)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(LLVM REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)


add_executable(rylang v2/rylang/include/rylang/ast/entity_ast.hpp v2/rylang/include/rylang/ast/entity_base_ast.hpp v2/rylang/sources/ast/entity_ast.cpp v2/rylang/include/rylang/ast/variable_ast.hpp v2/rylang/include/rylang/ast/function_entity_ast.hpp v2/rylang/include/rylang/data/entity_category.hpp)
add_library(graphlib INTERFACE v2/include/rpnx/graph_solver.hpp )
add_library(catch2 catch2/catch2/catch2.hpp catch2/catch2.cpp )
target_include_directories(catch2 PUBLIC catch2/)
target_include_directories(graphlib INTERFACE ${CMAKE_SOURCE_DIR}/v2/include)

add_executable(graphlib_tests v2/tests.cpp)
target_sources(rylang PRIVATE v2/rylang/sources/rylang_compiler.cpp
v2/rylang/sources/ast/type_ref_ast.cpp
 v2/rylang/include/rylang/compiler.hpp
  v2/rylang/include/rylang/compiler_fwd.hpp
        v2/rylang/sources/res/filelist_resolver.cpp
  v2/rylang/include/rylang/compiler_fwd.hpp v2/rylang/include/rylang/res/file_ast_resolver.hpp v2/rylang/include/rylang/ast/member_variable_ast.hpp v2/rylang/include/rylang/ast/file_ast.hpp
   v2/rylang/include/rylang/res/filelist_resolver.hpp v2/rylang/include/rylang/filelist.hpp v2/rylang/sources/main.cpp v2/rylang/include/rylang/res/file_content_resolver.hpp v2/rylang/sources/res/file_content_resolver.cpp v2/rylang/sources/res/file_ast_resolver.cpp v2/rylang/include/rylang/ast/class_ast.hpp v2/rylang/include/rylang/collector.hpp v2/rylang/include/rylang/ast/function_ast.hpp v2/rylang/include/rylang/ast/type_ref_ast.hpp v2/include/rpnx/value.hpp v2/rylang/include/rylang/ast/symbol_ref_ast.hpp v2/rylang/include/rylang/parser.hpp v2/rylang/include/rylang/ex/unexpected_eof.hpp v2/rylang/include/rylang/ast/null_object_ast.hpp v2/rylang/include/rylang/ast/null_object_ast.hpp
        v2/rylang/include/rylang/ex/unexpected_eof.hpp v2/rylang/include/rylang/data/class_list.hpp v2/rylang/include/rylang/res/class_list_resolver.hpp v2/rylang/sources/res/class_list_resolver.cpp v2/rylang/sources/res/classes_per_file_resolver.cpp v2/rylang/include/rylang/res/classes_per_file_resolver.hpp v2/rylang/include/rylang/ast/namespace_ast.hpp)
target_link_libraries(rylang PUBLIC graphlib)
target_include_directories(rylang PUBLIC v2/rylang/include)
target_link_libraries(graphlib_tests PUBLIC graphlib catch2)


get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${targets}")


# link to the llvm libraries
#target_link_libraries(ryanscript1031 PUBLIC ${LLVM_LIBS})


add_executable(ryanscript1031 main.cpp parser.hpp ast.hpp value.hpp semantic_generator.hpp sst.hpp collector.hpp input_source.hpp lookup_sequence.hpp lir_function.hpp lir_type_index.hpp argparser.hpp dependency_resolver_chain.hpp compiler_fwd.hpp map_alg.hpp indexer.hpp compiler.hpp size_index_resolver.hpp type_to_typed_field_declaration_list_resolver_fwd.hpp type_to_typed_field_declaration_list_resolver.hpp global_lookup_resolver_fwd.hpp global_lookup_resolver.hpp untyped_field_declaration_list_resolver.hpp untyped_field_declaration_list_resolver_fwd.hpp)

# for each property that starts with LLVM, link to it
foreach (target IN LISTS targets)
    if (target MATCHES "^LLVM.*" AND NOT target MATCHES "^LLVMLineEditor$")
        target_link_libraries(ryanscript1031 PRIVATE ${target})
    endif ()
endforeach ()

target_include_directories(ryanscript1031 PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(ryanscript1031 PUBLIC Boost::boost LLVMIRReader LLVMCore LLVMSupport LLVMMC LLVMMCParser LLVMMCDisassembler LLVMTarget LLVMBinaryFormat LLVMDemangle LLVMMCJIT)
