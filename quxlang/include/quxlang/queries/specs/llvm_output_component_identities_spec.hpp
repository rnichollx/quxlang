// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_OUTPUT_COMPONENT_IDENTITIES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_OUTPUT_COMPONENT_IDENTITIES_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_llvm_catalog.hpp>
#include <quxlang/queries/output_steppings.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_output_component_identities_spec
    {
        using query = llvm_output_component_identities_query;
        using dependencies = rpnx::typelist< output_binary_information_query, output_llvm_catalog_query, output_steppings_query >;
    };

    rpnx::querygraph::coroutine< llvm_output_component_identities_spec > llvm_output_component_identities_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_OUTPUT_COMPONENT_IDENTITIES_SPEC_HEADER_GUARD
