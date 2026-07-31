// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_UNOPTIMIZED_LLVM_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_UNOPTIMIZED_LLVM_HEADER_GUARD

#include <quxlang/queries/output_llvm_input.hpp>

namespace quxlang
{
    struct output_unoptimized_llvm_query
    {
        static constexpr auto query_id = "output_unoptimized_llvm";
        using input_type = llvm_output_query_input;
        using output_type = std::string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_UNOPTIMIZED_LLVM_HEADER_GUARD
