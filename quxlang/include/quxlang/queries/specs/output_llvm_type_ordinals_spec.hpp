// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_TYPE_ORDINALS_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_TYPE_ORDINALS_HEADER_GUARD
#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/output_llvm_catalog.hpp>
#include <quxlang/queries/output_llvm_type_ordinals.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Collects materialized types once from the output component catalogs. */
    struct output_llvm_type_ordinals_spec
    {
        using query = output_llvm_type_ordinals_query;
        using dependencies = rpnx::typelist< output_llvm_catalog_query, llvm_output_component_identities_query >;
    };
    /** Assigns deterministic dense type ordinals shared by every compilation unit. */
    rpnx::querygraph::coroutine< output_llvm_type_ordinals_spec > output_llvm_type_ordinals_impl(std::string input);
} // namespace quxlang
#endif
