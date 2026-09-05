// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD

#include <quxlang/blake2b.hpp>
#include <quxlang/llvm-backend-types.hpp>

#include <cstddef>
#include <cstdint>
#include <rpnx/macros.hpp>
#include <string>

/// Selects the dependency root represented by one independently compiled LLVM object.
RPNX_ENUM(quxlang, llvm_output_component, std::uint8_t, early_init, main_program, post_detect);

namespace quxlang
{
    /** Selects the non-procedure portion of a partitioned component. */
    struct llvm_support_data_unit
    {
        RPNX_MEMBER_METADATA(llvm_support_data_unit);
    };
    /** Selects a whole component, support data, or one procedure definition. */
    using llvm_compilation_unit_selector = rpnx::variant< std::monostate, llvm_support_data_unit, type_symbol >;

    /// Selects one configured output and optional stepped component for LLVM generation.
    struct llvm_output_query_input
    {
        std::string output_name;
        std::size_t stepping_index = 0;
        /// Selects the roots and ownership rules included in the compilation packet.
        llvm_output_component component = llvm_output_component::early_init;

        /// Whole-component discovery uses monostate; procedure units use canonical symbols.
        llvm_compilation_unit_selector unit;

        RPNX_MEMBER_METADATA(llvm_output_query_input, output_name, stepping_index, component, unit);
    };

    /** Returns the stable diagnostic filename component for one compiled object identity. */
    inline auto llvm_output_component_name(llvm_output_query_input const& input) -> std::string
    {
        std::string suffix;
        if (input.unit.type_is< llvm_support_data_unit >())
        {
            suffix = ".support";
        }
        if (input.unit.type_is< type_symbol >())
        {
            // The artifact writer also includes the canonical procedure name.
            std::string name = to_string(input.unit.get_as< type_symbol >());
            suffix = ".procedure-" + blake2b::hex(std::as_bytes(std::span(name.data(), name.size()))).substr(0, 32);
        }
        switch (input.component)
        {
        case llvm_output_component::early_init:
            return "early-init" + suffix;
        case llvm_output_component::main_program:
            return "main-X" + std::to_string(input.stepping_index) + suffix;
        case llvm_output_component::post_detect:
            return "post-detect-X" + std::to_string(input.stepping_index) + suffix;
        }
        throw compiler_bug("Unknown LLVM output component");
    }
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_LLVM_INPUT_HEADER_GUARD
