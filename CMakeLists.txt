cmake_minimum_required(VERSION 3.22)
project(quxlang)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(CTest)

find_package(RPNXCow REQUIRED)
find_package(benchmark REQUIRED)

add_subdirectory(rpnx)

add_subdirectory(quxlang)
