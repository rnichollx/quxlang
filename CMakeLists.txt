cmake_minimum_required(VERSION 3.22)
project(ryanscript1031)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(LLVM REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)


add_library(catch2 catch2/catch2/catch2.hpp catch2/catch2.cpp )
target_include_directories(catch2 PUBLIC catch2/)

add_executable(graphlib_tests rylang/tests.cpp)
add_subdirectory(rylang)
target_link_libraries(graphlib_tests PUBLIC graphlib catch2)


get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${targets}")


# link to the llvm libraries
#target_link_libraries(ryanscript1031 PUBLIC ${LLVM_LIBS})



# for each property that starts with LLVM, link to it
foreach (target IN LISTS targets)
    if (target MATCHES "^LLVM.*" AND NOT target MATCHES "^LLVMLineEditor$")
    #    target_link_libraries(ryanscript1031 PRIVATE ${target})
    endif ()
endforeach ()

#target_include_directories(ryanscript1031 PRIVATE ${LLVM_INCLUDE_DIRS})
#target_link_libraries(ryanscript1031 PUBLIC Boost::boost LLVMIRReader LLVMCore LLVMSupport LLVMMC LLVMMCParser LLVMMCDisassembler LLVMTarget LLVMBinaryFormat LLVMDemangle LLVMMCJIT)
