if (CMAKE_IS_TOP_LEVEL_PROJECT)
    cmake_minimum_required(VERSION 3.22)
    project(RPNXGraphLib)
endif ()

add_library(graphlib INTERFACE include/rpnx/resolver_utilities.hpp include/rpnx/value.hpp
        include/rpnx/variant.hpp
        include/rpnx/string.hpp
        include/rpnx/compare.hpp
        include/rpnx/serializer.hpp
        include/rpnx/range.hpp
        include/rpnx/resolver2.hpp
        include/rpnx/compat/variant_serializer.hpp)
target_include_directories(graphlib INTERFACE include/)
add_executable(graphlib_tests tests.cpp)
target_link_libraries(graphlib_tests PRIVATE Boost::boost graphlib)