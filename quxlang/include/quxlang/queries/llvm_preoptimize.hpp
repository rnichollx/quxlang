// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_PREOPTIMIZE_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_PREOPTIMIZE_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

namespace quxlang
{
    /** Returns a verified compilation unit, sharing unchanged cached function modules. */
    struct llvm_preoptimize_query
    {
        static constexpr auto query_id = "llvm_preoptimize";
        using input_type = llvm_output_query_input;
        using output_type = rpnx::cow< llvm_backend::llvm_preoptimized_unit >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_PREOPTIMIZE_HEADER_GUARD
