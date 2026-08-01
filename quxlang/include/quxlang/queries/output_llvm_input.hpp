// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD

#include <quxlang/llvm-backend-types.hpp>

#include <cstddef>
#include <cstdint>
#include <rpnx/macros.hpp>
#include <string>

/// Selects the dependency root represented by one independently compiled LLVM object.
RPNX_ENUM(quxlang, llvm_output_component, std::uint8_t, early_init, main_program, post_detect);

namespace quxlang
{
    /// Selects one configured output and optional stepped component for LLVM generation.
    struct llvm_output_query_input
    {
        std::string output_name;
        std::size_t stepping_index = 0;
        /// Selects the roots and ownership rules included in the compilation packet.
        llvm_output_component component = llvm_output_component::early_init;

        RPNX_MEMBER_METADATA(llvm_output_query_input, output_name, stepping_index, component);
    };

    struct output_llvm_input_query
    {
        static constexpr auto query_id = "output_llvm_input";
        using input_type = llvm_output_query_input;
        using output_type = llvm_backend::llvm_compilable_unit;
    };

    /** Returns the stable diagnostic filename component for one compiled object identity. */
    inline auto llvm_output_component_name(llvm_output_query_input const& input) -> std::string
    {
        switch (input.component)
        {
        case llvm_output_component::early_init:
            return "early-init";
        case llvm_output_component::main_program:
            return "main-X" + std::to_string(input.stepping_index);
        case llvm_output_component::post_detect:
            return "post-detect-X" + std::to_string(input.stepping_index);
        }
        throw compiler_bug("Unknown LLVM output component");
    }
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
