//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef RPNX_QUXLANG_ASM_HEADER
#define RPNX_QUXLANG_ASM_HEADER

#include <string>
#include <vector>

#include <rpnx/compare.hpp>
#include <rpnx/variant.hpp>

namespace quxlang
{

    struct asm_instruction
    {
        std::string opcode_mnemonic;
        std::vector< std::string > operands;

        auto operator<=>(const asm_instruction& other) const
        {
            return rpnx::compare(opcode_mnemonic, other.opcode_mnemonic, operands, other.operands);
        }
    };

    struct asm_label
    {
        std::string name;

        auto operator<=>(const asm_label&other) const
        {
            return rpnx::compare(name, other.name);
        }
    };

    using asm_statement = rpnx::variant< asm_instruction, asm_label >;

    struct asm_procedure
    {
        std::string name;
        std::vector< asm_statement > instructions;
    };


} // namespace quxlang

#endif // RPNX_QUXLANG_ASM_HEADER