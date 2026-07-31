// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_MAIN_PREOPTIMIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_MAIN_PREOPTIMIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_main_preoptimize.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// Connects main-program LLVM generation to the aggregate LLVM compilation query.
    struct llvm_main_preoptimize_spec
    {
        using query = llvm_main_preoptimize_query;
        using dependencies = rpnx::typelist< output_llvm_input_query >;
    };

    /** Generates pre-optimization LLVM bitcode rooted at the configured main functanoid. */
    rpnx::querygraph::coroutine< llvm_main_preoptimize_spec > llvm_main_preoptimize_impl(llvm_main_query_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_MAIN_PREOPTIMIZE_SPEC_HEADER_GUARD
