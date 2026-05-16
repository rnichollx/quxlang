cmake_minimum_required(VERSION 4.12)


project(QuxlangLLVMBackend)

find_package(Quxlang REQUIRED)

add_library(quxlang-llvm-backend)

target_link_libraries(quxlang-llvm-backend PUBLIC Quxlang::quxlang)



