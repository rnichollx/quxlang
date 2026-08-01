// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_PREOPTIMIZE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_PREOPTIMIZE_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_compiler_builtin_manifest.hpp>
#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/llvm_preoptimize.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_preoptimize_spec
    {
        using query = llvm_preoptimize_query;
        using dependencies = rpnx::typelist<
            output_llvm_input_query,
            llvm_output_component_identities_query,
            llvm_compiler_builtin_manifest_query >;
    };

    rpnx::querygraph::coroutine< llvm_preoptimize_spec > llvm_preoptimize_impl(llvm_output_query_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_PREOPTIMIZE_SPEC_HEADER_GUARD
