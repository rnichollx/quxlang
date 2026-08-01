// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_COMPILED_OUTPUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_COMPILED_OUTPUT_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_compiled_output.hpp>
#include <quxlang/queries/llvm_output_component_identities.hpp>
#include <quxlang/queries/llvm_post_codegen.hpp>
#include <quxlang/queries/llvm_postoptimize.hpp>
#include <quxlang/queries/llvm_preoptimize.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_compiled_output_spec
    {
        using query = llvm_compiled_output_query;
        using dependencies = rpnx::typelist<
            llvm_preoptimize_query,
            llvm_postoptimize_query,
            llvm_output_component_identities_query,
            llvm_post_codegen_query >;
    };

    rpnx::querygraph::coroutine< llvm_compiled_output_spec > llvm_compiled_output_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_COMPILED_OUTPUT_SPEC_HEADER_GUARD
