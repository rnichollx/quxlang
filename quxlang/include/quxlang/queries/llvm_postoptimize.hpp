// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_POSTOPTIMIZE_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_POSTOPTIMIZE_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

namespace quxlang
{
    /** Optimizes one configured LLVM component without performing native code generation. */
    struct llvm_postoptimize_query
    {
        static constexpr auto query_id = "llvm_postoptimize";
        using input_type = llvm_output_query_input;
        using output_type = llvm_backend::llvm_postoptimized_unit;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_POSTOPTIMIZE_HEADER_GUARD
