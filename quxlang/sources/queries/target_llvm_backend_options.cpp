// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/target_llvm_backend_options_spec.hpp>

rpnx::querygraph::coroutine< quxlang::target_llvm_backend_options_spec > quxlang::target_llvm_backend_options_impl(std::monostate)
{
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});
    co_return target_config.llvm_options;
}
