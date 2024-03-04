if (CMAKE_IS_TOP_LEVEL_PROJECT)
    cmake_minimum_required(VERSION 3.22)
    project(RPNXGraphLib)
endif ()

add_library(graphlib INTERFACE include/rpnx/resolver_utilities.hpp include/rpnx/value.hpp
        include/rpnx/variant.hpp)
target_include_directories(graphlib INTERFACE include/)
add_executable(graphlib_tests tests.cpp)