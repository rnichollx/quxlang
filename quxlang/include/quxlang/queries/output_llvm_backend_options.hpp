// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_LLVM_BACKEND_OPTIONS_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LLVM_BACKEND_OPTIONS_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <string>

namespace quxlang
{
    /// output_llvm_backend_options_query returns effective LLVM backend options for one output.
    struct output_llvm_backend_options_query
    {
        static constexpr auto query_id = "output_llvm_backend_options";
        using input_type = std::string;
        using output_type = backend_llvm_options;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_LLVM_BACKEND_OPTIONS_HEADER_GUARD
