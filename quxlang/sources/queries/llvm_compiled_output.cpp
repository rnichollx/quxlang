// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/llvm-backend.hpp>
#include <quxlang/queries/specs/llvm_compiled_output_spec.hpp>

rpnx::querygraph::coroutine< quxlang::llvm_compiled_output_spec > quxlang::llvm_compiled_output_impl(std::string input)
{
    llvm_backend::llvm_compilable_unit const compilable_unit = co_await rpnx::querygraph::request< output_llvm_input_query >(std::move(input));
    llvm::llvm_backend backend;
    co_return backend.compile(compilable_unit);
}
