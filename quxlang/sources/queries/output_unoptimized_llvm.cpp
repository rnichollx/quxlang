// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/llvm_compiled_output.hpp>
#include <quxlang/queries/specs/output_unoptimized_llvm_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_unoptimized_llvm_spec > quxlang::output_unoptimized_llvm_impl(std::string input)
{
    llvm_backend::llvm_compiled_unit const compiled = co_await rpnx::querygraph::request< llvm_compiled_output_query >(std::move(input));
    co_return compiled.llvm_ir_text;
}
