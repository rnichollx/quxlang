cmake_minimum_required(VERSION 3.22)
project(ryanscript1031)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(LLVM REQUIRED CONFIG)
find_package(Boost REQUIRED COMPONENTS filesystem)

get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${targets}")


# link to the llvm libraries
#target_link_libraries(ryanscript1031 PUBLIC ${LLVM_LIBS})


add_executable(ryanscript1031 main.cpp parser.hpp ast.hpp value.hpp semantic_generator.hpp sst.hpp collector.hpp input_source.hpp lookup_sequence.hpp lir_function.hpp lir_type_index.hpp argparser.hpp dependency_resolver_chain.hpp compiler.hpp map_alg.hpp)

# for each property that starts with LLVM, link to it
foreach (target IN LISTS targets)
    if (target MATCHES "^LLVM.*" AND NOT target MATCHES "^LLVMLineEditor$")
        target_link_libraries(ryanscript1031 PRIVATE ${target})
    endif ()
endforeach ()

target_include_directories(ryanscript1031 PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(ryanscript1031 PUBLIC Boost::boost LLVMIRReader LLVMCore LLVMSupport LLVMMC LLVMMCParser LLVMMCDisassembler LLVMTarget LLVMBinaryFormat LLVMDemangle LLVMMCJIT)
