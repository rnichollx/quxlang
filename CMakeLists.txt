cmake_minimum_required(VERSION 3.22)
project(quxlang)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
#find_package(LLVM REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)

add_subdirectory(rpnx)

add_subdirectory(quxlang)


get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${targets}")


# link to the llvm libraries
#target_link_libraries(quxlang PUBLIC ${LLVM_LIBS})



# for each property that starts with LLVM, link to it
foreach (target IN LISTS targets)
    if (target MATCHES "^LLVM.*" AND NOT target MATCHES "^LLVMLineEditor$")
    #    target_link_libraries(quxlang PRIVATE ${target})
    endif ()
endforeach ()
