// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_BACKEND_OPTIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_BACKEND_OPTIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_llvm_backend_options.hpp>
#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/target_llvm_backend_options.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_llvm_backend_options_spec
    {
        using query = output_llvm_backend_options_query;
        using dependencies = rpnx::typelist< output_binary_information_query, target_configuration_query, target_llvm_backend_options_query >;
    };

    rpnx::querygraph::coroutine< output_llvm_backend_options_spec > output_llvm_backend_options_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_BACKEND_OPTIONS_SPEC_HEADER_GUARD
