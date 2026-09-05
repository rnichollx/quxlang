// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_output_binary_artifact.hpp>
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_llvm_backend_options.hpp>
#include <quxlang/queries/output_llvm_catalog.hpp>
#include <quxlang/queries/target_configuration.hpp>

#include <quxlang/queries/llvm_compilation_unit_identities.hpp>
#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/llvm_post_codegen.hpp>
#include <quxlang/queries/llvm_postoptimize.hpp>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_output_binary_artifact_spec
    {
        using query = llvm_output_binary_artifact_query;
        using dependencies = rpnx::typelist< output_binary_information_query, target_configuration_query, output_llvm_backend_options_query, output_llvm_catalog_query, llvm_compilation_unit_identities_query, llvm_output_component_identities_query, llvm_post_codegen_query, llvm_postoptimize_query >;
    };

    rpnx::querygraph::coroutine< llvm_output_binary_artifact_spec > llvm_output_binary_artifact_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_OUTPUT_BINARY_ARTIFACT_SPEC_HEADER_GUARD
