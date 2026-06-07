// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_COMPILED_OUTPUT_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_COMPILED_OUTPUT_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>

#include <string>

namespace quxlang
{
    /// llvm_compiled_output_query compiles one output module to LLVM objects.
    struct llvm_compiled_output_query
    {
        static constexpr auto query_id = "llvm_compiled_output";
        using input_type = std::string;
        using output_type = llvm_backend::llvm_compiled_unit;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_COMPILED_OUTPUT_HEADER_GUARD
