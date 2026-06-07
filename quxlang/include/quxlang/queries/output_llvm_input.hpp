// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>

#include <string>

namespace quxlang
{
    struct output_llvm_input_query
    {
        static constexpr auto query_id = "output_llvm_input";
        using input_type = std::string;
        using output_type = llvm_backend::llvm_compilable_unit;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
