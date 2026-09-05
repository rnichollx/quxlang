// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/output_llvm_backend_options_spec.hpp>
rpnx::querygraph::coroutine< quxlang::output_llvm_backend_options_spec > quxlang::output_llvm_backend_options_impl(std::string input)
{
    output_build_settings settings = co_await rpnx::querygraph::request< output_build_settings_query >(std::move(input));
    co_return backend_llvm_options{settings.llvm_build_type};
}
