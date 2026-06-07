// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD

#include <quxlang/queries/output_binary_artifact.hpp>

#include <string>

namespace quxlang
{
    /// llvm_output_binary_artifact_query returns a final artifact produced by the LLVM backend.
    struct llvm_output_binary_artifact_query
    {
        static constexpr auto query_id = "llvm_output_binary_artifact";
        using input_type = std::string;
        using output_type = output_binary_artifact;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD
