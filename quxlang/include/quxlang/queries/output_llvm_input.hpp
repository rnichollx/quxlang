// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>

#include <cstddef>
#include <cstdint>
#include <rpnx/macros.hpp>
#include <string>

/// Selects which roots contribute to an aggregate LLVM compilation unit.
RPNX_ENUM(quxlang, llvm_output_component, std::uint8_t, complete_output, main_program);

namespace quxlang
{
    /// Selects one configured output and program stepping for LLVM generation.
    struct llvm_output_query_input
    {
        std::string output_name;
        std::size_t stepping_index = 0;
        /// Selects the roots included in the compilation unit.
        llvm_output_component component = llvm_output_component::complete_output;

        RPNX_MEMBER_METADATA(llvm_output_query_input, output_name, stepping_index, component);
    };

    struct output_llvm_input_query
    {
        static constexpr auto query_id = "output_llvm_input";
        using input_type = llvm_output_query_input;
        using output_type = llvm_backend::llvm_compilable_unit;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
