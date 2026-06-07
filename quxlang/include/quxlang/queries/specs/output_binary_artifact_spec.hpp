// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_output_binary_artifact.hpp>
#include <quxlang/queries/output_binary_artifact.hpp>
#include <quxlang/queries/target_backend.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_binary_artifact_spec
    {
        using query = output_binary_artifact_query;
        using dependencies = rpnx::typelist< target_backend_query, llvm_output_binary_artifact_query >;
    };

    rpnx::querygraph::coroutine< output_binary_artifact_spec > output_binary_artifact_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD
