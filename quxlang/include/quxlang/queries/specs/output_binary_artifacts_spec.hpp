// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_ARTIFACTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_ARTIFACTS_SPEC_HEADER_GUARD

#include <quxlang/queries/output_binaries_information.hpp>
#include <quxlang/queries/output_binary_artifact.hpp>
#include <quxlang/queries/output_binary_artifacts.hpp>
#include <quxlang/queries/run_static_tests.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_binary_artifacts_spec
    {
        using query = output_binary_artifacts_query;
        using dependencies = rpnx::typelist< run_static_tests_query, output_binaries_information_query, output_binary_artifact_query >;
    };

    rpnx::querygraph::coroutine< output_binary_artifacts_spec > output_binary_artifacts_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_ARTIFACTS_SPEC_HEADER_GUARD
