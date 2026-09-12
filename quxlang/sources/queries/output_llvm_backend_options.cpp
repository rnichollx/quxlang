// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/queries/specs/output_llvm_backend_options_spec.hpp>
rpnx::querygraph::coroutine< quxlang::output_llvm_backend_options_spec > quxlang::output_llvm_backend_options_impl(std::string input)
{
    output_build_settings settings = co_await rpnx::querygraph::request< output_build_settings_query >(input);
    backend_llvm_options options = co_await rpnx::querygraph::request< target_llvm_backend_options_query >(std::monostate{});
    source_bundle const& bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    output_config const& output = bundle.outputs.at(input);
    if (output.llvm_options.has_value() && output.llvm_options->enable_strict_aliasing.has_value())
    {
        options.enable_strict_aliasing = output.llvm_options->enable_strict_aliasing;
    }
    options.build_type = settings.llvm_build_type;
    options.enable_strict_aliasing = options.enable_strict_aliasing.value_or(settings.llvm_build_type != build_type::debug && settings.llvm_build_type != build_type::quick);
    co_return options;
}
