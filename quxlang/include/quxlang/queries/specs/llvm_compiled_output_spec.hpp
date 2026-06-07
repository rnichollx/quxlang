// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LLVM_COMPILED_OUTPUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LLVM_COMPILED_OUTPUT_SPEC_HEADER_GUARD

#include <quxlang/queries/llvm_compiled_output.hpp>
#include <quxlang/queries/output_llvm_input.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct llvm_compiled_output_spec
    {
        using query = llvm_compiled_output_query;
        using dependencies = rpnx::typelist< output_llvm_input_query >;
    };

    rpnx::querygraph::coroutine< llvm_compiled_output_spec > llvm_compiled_output_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LLVM_COMPILED_OUTPUT_SPEC_HEADER_GUARD
