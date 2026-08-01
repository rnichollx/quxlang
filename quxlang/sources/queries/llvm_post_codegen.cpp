// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/llvm-backend.hpp>
#include <quxlang/queries/specs/llvm_post_codegen_spec.hpp>

#include <utility>

rpnx::querygraph::coroutine< quxlang::llvm_post_codegen_spec > quxlang::llvm_post_codegen_impl(llvm_output_query_input input)
{
    llvm_backend::llvm_postoptimized_unit const& postoptimized =
        co_await rpnx::querygraph::request< llvm_postoptimize_query >(std::move(input));

    llvm_backend::llvm_backend backend;
    co_return backend.post_codegen(postoptimized);
}
