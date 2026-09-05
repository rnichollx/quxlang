// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_llvm_backend_options_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_llvm_backend_options_spec > quxlang::output_llvm_backend_options_impl(std::string input)
{
    co_await rpnx::querygraph::request< output_binary_information_query >(input);

    backend_llvm_options const target_options = co_await rpnx::querygraph::request< target_llvm_backend_options_query >(std::monostate{});
    source_bundle const& bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    output_config const& config = bundle.outputs.at(input);
    if (config.llvm_options.has_value())
    {
        co_return *config.llvm_options;
    }

    co_return target_options;
}
