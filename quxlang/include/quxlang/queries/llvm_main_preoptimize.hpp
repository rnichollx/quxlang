// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_MAIN_PREOPTIMIZE_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_MAIN_PREOPTIMIZE_HEADER_GUARD

#include <quxlang/queries/llvm_main_query_input.hpp>

#include <cstddef>
#include <vector>

namespace quxlang
{
    /// Produces pre-optimization LLVM bitcode for the main-reachable program component.
    struct llvm_main_preoptimize_query
    {
        static constexpr auto query_id = "llvm_main_preoptimize";
        using input_type = llvm_main_query_input;
        using output_type = std::vector< std::byte >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_MAIN_PREOPTIMIZE_HEADER_GUARD
