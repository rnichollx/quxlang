// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_UNOPTIMIZED_LLVM_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_UNOPTIMIZED_LLVM_SPEC_HEADER_GUARD

#include <quxlang/queries/output_unoptimized_llvm.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_unoptimized_llvm_spec
    {
        using query = output_unoptimized_llvm_query;
        using dependencies = rpnx::typelist<>;
    };

    rpnx::querygraph::coroutine< output_unoptimized_llvm_spec > output_unoptimized_llvm_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_UNOPTIMIZED_LLVM_SPEC_HEADER_GUARD
