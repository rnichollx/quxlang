// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_INPUT_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_INPUT_SPEC_HEADER_GUARD

#include <quxlang/queries/output_llvm_input.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_llvm_input_spec
    {
        using query = output_llvm_input_query;
        using dependencies = rpnx::typelist<>;
    };

    rpnx::querygraph::coroutine< output_llvm_input_spec > output_llvm_input_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_LLVM_INPUT_SPEC_HEADER_GUARD
