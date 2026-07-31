// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_MAIN_POSTOPTIMIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_MAIN_POSTOPTIMIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_main_postoptimize.hpp>
#include <quxlang/queries/llvm_main_preoptimize.hpp>
#include <quxlang/queries/output_llvm_backend_options.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// Connects post-optimization main LLVM generation to the aggregate LLVM compilation query.
    struct llvm_main_postoptimize_spec
    {
        using query = llvm_main_postoptimize_query;
        using dependencies = rpnx::typelist< llvm_main_preoptimize_query, output_llvm_backend_options_query >;
    };

    /** Generates configured post-optimization LLVM bitcode rooted at the configured main functanoid. */
    rpnx::querygraph::coroutine< llvm_main_postoptimize_spec > llvm_main_postoptimize_impl(llvm_main_query_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_MAIN_POSTOPTIMIZE_SPEC_HEADER_GUARD
