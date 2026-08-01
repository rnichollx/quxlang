// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_POST_CODEGEN_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_POST_CODEGEN_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_post_codegen.hpp>
#include <quxlang/queries/llvm_postoptimize.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_post_codegen_spec
    {
        using query = llvm_post_codegen_query;
        using dependencies = rpnx::typelist< llvm_postoptimize_query >;
    };

    rpnx::querygraph::coroutine< llvm_post_codegen_spec > llvm_post_codegen_impl(llvm_output_query_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_POST_CODEGEN_SPEC_HEADER_GUARD
