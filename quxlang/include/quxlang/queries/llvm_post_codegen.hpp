// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_POST_CODEGEN_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_POST_CODEGEN_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

namespace quxlang
{
    /** Emits one independently linkable native object from the post-optimized component. */
    struct llvm_post_codegen_query
    {
        static constexpr auto query_id = "llvm_post_codegen";
        using input_type = llvm_output_query_input;
        using output_type = llvm_backend::llvm_post_codegen_unit;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_POST_CODEGEN_HEADER_GUARD
