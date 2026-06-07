// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_OPTIMIZED_LLVM_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_OPTIMIZED_LLVM_HEADER_GUARD

#include <string>

namespace quxlang
{
    struct output_optimized_llvm_query
    {
        static constexpr auto query_id = "output_optimized_llvm";
        using input_type = std::string;
        using output_type = std::string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_OPTIMIZED_LLVM_HEADER_GUARD
