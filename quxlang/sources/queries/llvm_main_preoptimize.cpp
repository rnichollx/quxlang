// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/llvm_main_preoptimize_spec.hpp>
#include <quxlang/llvm-backend.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

#include <utility>

rpnx::querygraph::coroutine< quxlang::llvm_main_preoptimize_spec > quxlang::llvm_main_preoptimize_impl(llvm_main_query_input input)
{
    llvm_backend::llvm_compilable_unit const compilable = co_await rpnx::querygraph::request< output_llvm_input_query >(llvm_output_query_input{
        .output_name = std::move(input.output_name),
        .stepping_index = input.stepping_index,
        .component = llvm_output_component::main_program,
    });
    llvm_backend::llvm_backend backend;
    co_return backend.preoptimize(compilable);
}
