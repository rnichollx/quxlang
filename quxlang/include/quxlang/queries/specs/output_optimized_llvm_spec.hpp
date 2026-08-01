// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_OPTIMIZED_LLVM_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_OPTIMIZED_LLVM_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/llvm_postoptimize.hpp>
#include <quxlang/queries/output_optimized_llvm.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_optimized_llvm_spec
    {
        using query = output_optimized_llvm_query;
        using dependencies = rpnx::typelist< llvm_output_component_identities_query, llvm_postoptimize_query >;
    };

    rpnx::querygraph::coroutine< output_optimized_llvm_spec > output_optimized_llvm_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_OPTIMIZED_LLVM_SPEC_HEADER_GUARD
