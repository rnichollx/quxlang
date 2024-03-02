//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef RPNX_QUXLANG_ASM_HEADER
#define RPNX_QUXLANG_ASM_HEADER

#include <string>
namespace quxlang
{
    struct asm_instruction
    {
        std::string opcode_mnemonic;
        std::vector< std::string > operands;
    };

    struct asm_procedure
    {
        std::string name;
        std::vector< asm_instruction > instructions;
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_ASM_HEADER
