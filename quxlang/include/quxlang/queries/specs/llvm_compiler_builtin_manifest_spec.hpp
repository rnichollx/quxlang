// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_COMPILER_BUILTIN_MANIFEST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_COMPILER_BUILTIN_MANIFEST_SPEC_HEADER_GUARD

#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/llvm_compiler_builtin_manifest.hpp>
#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/output_llvm_catalog.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_compiler_builtin_manifest_spec
    {
        using query = llvm_compiler_builtin_manifest_query;
        using dependencies = rpnx::typelist< llvm_output_component_identities_query, output_llvm_catalog_query, global_init_type_query >;
    };

    rpnx::querygraph::coroutine< llvm_compiler_builtin_manifest_spec > llvm_compiler_builtin_manifest_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_COMPILER_BUILTIN_MANIFEST_SPEC_HEADER_GUARD
