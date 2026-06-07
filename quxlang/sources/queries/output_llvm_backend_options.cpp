// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_llvm_backend_options_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_llvm_backend_options_spec > quxlang::output_llvm_backend_options_impl(std::string input)
{
    co_await rpnx::querygraph::request< output_binary_information_query >(input);

    backend_llvm_options const target_options = co_await rpnx::querygraph::request< target_llvm_backend_options_query >(std::monostate{});
    target_configuration const& target_config = co_await rpnx::querygraph::request< target_configuration_query >(std::monostate{});

    if (target_config.outputs.has_value())
    {
        std::map< std::string, output_config >::const_iterator output_iter = target_config.outputs->find(input);
        if (output_iter != target_config.outputs->end() && output_iter->second.llvm_options.has_value())
        {
            co_return *output_iter->second.llvm_options;
        }
    }

    co_return target_options;
}
