if (CMAKE_IS_TOP_LEVEL_PROJECT)
    cmake_minimum_required(VERSION 3.22)
    project(RPNXGraphLib)
endif ()

find_package(RPNXMetalib REQUIRED)
find_package(RPNXSerialization REQUIRED)

add_library(graphlib INTERFACE include/rpnx/resolver_utilities.hpp include/rpnx/value.hpp
        include/rpnx/string.hpp
        include/rpnx/range.hpp
        include/rpnx/resolver2.hpp
        include/rpnx/inttypes.h
        include/rpnx/uint64_base.hpp
        include/rpnx/demangle.hpp)
target_include_directories(graphlib INTERFACE include/)


target_link_libraries(graphlib INTERFACE RPNX::serialization RPNX::metalib)

