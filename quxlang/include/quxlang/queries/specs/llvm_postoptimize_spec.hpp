// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_POSTOPTIMIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_POSTOPTIMIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_postoptimize.hpp>
#include <quxlang/queries/llvm_preoptimize.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_postoptimize_spec
    {
        using query = llvm_postoptimize_query;
        using dependencies = rpnx::typelist< llvm_preoptimize_query >;
    };

    rpnx::querygraph::coroutine< llvm_postoptimize_spec > llvm_postoptimize_impl(llvm_output_query_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_POSTOPTIMIZE_SPEC_HEADER_GUARD
