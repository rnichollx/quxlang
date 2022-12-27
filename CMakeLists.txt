cmake_minimum_required(VERSION 3.23)
project(ryanscript1031)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(LLVM REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)

get_property(targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${targets}")

add_executable(ryanscript1031 main.cpp )

target_link_libraries(ryanscript1031 PUBLIC LLVM::LLVM)
