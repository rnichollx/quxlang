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
        include/rpnx/compat/variant_serializer.hpp
        include/rpnx/metadata.hpp
        include/rpnx/inttypes.h
        include/rpnx/uint64_base.hpp
        include/rpnx/demangle.hpp)
target_include_directories(graphlib INTERFACE include/)