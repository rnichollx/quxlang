// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LLVM_MAIN_QUERY_INPUT_HEADER_GUARD
#define QUXLANG_QUERIES_LLVM_MAIN_QUERY_INPUT_HEADER_GUARD

#include <cstddef>
#include <rpnx/macros.hpp>
#include <string>

namespace quxlang
{
    /// Selects one configured output and program stepping for main-program LLVM generation.
    struct llvm_main_query_input
    {
        std::string output_name;
        std::size_t stepping_index = 0;

        RPNX_MEMBER_METADATA(llvm_main_query_input, output_name, stepping_index);
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LLVM_MAIN_QUERY_INPUT_HEADER_GUARD
