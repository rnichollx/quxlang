// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CORTADO_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CORTADO_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD

#include <quxlang/queries/cortado_output_binary_artifact.hpp>
#include <quxlang/queries/output_cortado_input.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Query specification for final Cortado JAR generation. */
    struct cortado_output_binary_artifact_spec
    {
        using query = cortado_output_binary_artifact_query;
        using dependencies = rpnx::typelist< output_cortado_input_query >;
    };

    /** Implements final Cortado JAR generation. */
    auto cortado_output_binary_artifact_impl(std::string input) -> rpnx::querygraph::coroutine< cortado_output_binary_artifact_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CORTADO_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD
