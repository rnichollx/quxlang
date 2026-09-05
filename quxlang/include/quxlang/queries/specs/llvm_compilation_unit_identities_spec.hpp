// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_COMPILATION_UNIT_IDENTITIES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_COMPILATION_UNIT_IDENTITIES_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_compilation_unit_identities.hpp>
#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_llvm_catalog.hpp>
#include <quxlang/queries/output_steppings.hpp>

#include <quxlang/queries/output_llvm_backend_options.hpp>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_compilation_unit_identities_spec
    {
        using query = llvm_compilation_unit_identities_query;
        using dependencies = rpnx::typelist< llvm_output_component_identities_query, output_binary_information_query, output_llvm_catalog_query, output_llvm_backend_options_query, output_steppings_query >;
    };

    rpnx::querygraph::coroutine< llvm_compilation_unit_identities_spec > llvm_compilation_unit_identities_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_COMPILATION_UNIT_IDENTITIES_SPEC_HEADER_GUARD
