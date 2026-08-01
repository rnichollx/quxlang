// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/llvm-backend.hpp>
#include <quxlang/queries/specs/llvm_postoptimize_spec.hpp>

#include <utility>

rpnx::querygraph::coroutine< quxlang::llvm_postoptimize_spec > quxlang::llvm_postoptimize_impl(llvm_output_query_input input)
{
    llvm_backend::llvm_preoptimized_unit const& preoptimized =
        co_await rpnx::querygraph::request< llvm_preoptimize_query >(std::move(input));
    llvm_backend::llvm_backend backend;
    co_return backend.postoptimize(preoptimized);
}
