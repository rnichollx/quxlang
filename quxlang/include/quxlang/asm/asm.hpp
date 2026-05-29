// Copyright 2024, 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_ASM_ASM_HEADER_GUARD
#define QUXLANG_ASM_ASM_HEADER_GUARD

#include <string>
#include <vector>

#include <rpnx/compare.hpp>
#include <rpnx/variant.hpp>
#include <rpnx/macros.hpp>

namespace quxlang
{

    struct asm_instruction
    {
        std::string opcode_mnemonic;
        std::vector< std::string > operands;

        RPNX_MEMBER_METADATA(asm_instruction, opcode_mnemonic, operands);
    };

    struct asm_label
    {
        std::string name;

        RPNX_MEMBER_METADATA(asm_label, name);
    };

    using asm_statement = rpnx::variant< asm_instruction, asm_label >;

    struct asm_procedure
    {
        /// Target architecture tag as written in the source declaration, e.g. X64 or ARM.
        std::string architecture;
        /// Link-visible symbol name emitted for this assembly routine.
        std::string name;
        /// Ordered statements that form the body of this assembly routine.
        std::vector< asm_statement > instructions;

        RPNX_MEMBER_METADATA(asm_procedure, architecture, name, instructions);
    };


} // namespace quxlang

#endif // RPNX_QUXLANG_ASM_HEADER
