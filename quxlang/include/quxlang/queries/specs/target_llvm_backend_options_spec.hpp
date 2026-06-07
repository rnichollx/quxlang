// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TARGET_LLVM_BACKEND_OPTIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TARGET_LLVM_BACKEND_OPTIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/target_llvm_backend_options.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct target_llvm_backend_options_spec
    {
        using query = target_llvm_backend_options_query;
        using dependencies = rpnx::typelist< target_configuration_query >;
    };

    rpnx::querygraph::coroutine< target_llvm_backend_options_spec > target_llvm_backend_options_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TARGET_LLVM_BACKEND_OPTIONS_SPEC_HEADER_GUARD
