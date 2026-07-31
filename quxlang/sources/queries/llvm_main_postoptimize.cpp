// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/llvm_main_postoptimize_spec.hpp>
#include <quxlang/llvm-backend.hpp>

rpnx::querygraph::coroutine< quxlang::llvm_main_postoptimize_spec > quxlang::llvm_main_postoptimize_impl(llvm_main_query_input input)
{
    rpnx::querygraph::request< llvm_main_preoptimize_query > preoptimize_request(input);
    rpnx::querygraph::request< output_llvm_backend_options_query > options_request(input.output_name);
    co_yield rpnx::querygraph::dependency(preoptimize_request);
    co_yield rpnx::querygraph::dependency(options_request);

    std::vector< std::byte > const preoptimized = co_await preoptimize_request;
    backend_llvm_options const options = co_await options_request;
    if (options.mode == backend_llvm_mode::debug)
    {
        co_return preoptimized;
    }

    llvm_backend::llvm_backend backend;
    co_return backend.optimize(preoptimized);
}
