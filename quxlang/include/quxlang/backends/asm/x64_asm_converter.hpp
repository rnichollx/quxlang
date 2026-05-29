// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BACKENDS_ASM_X64_ASM_CONVERTER_HEADER_GUARD
#define QUXLANG_BACKENDS_ASM_X64_ASM_CONVERTER_HEADER_GUARD

#include "quxlang/asm/asm.hpp"
#include "quxlang/backends/asm/arm_asm_converter.hpp"
#include "quxlang/variant_utils.hpp"

#include <string>

namespace quxlang
{
    /**
     * Converts one x86-64 assembly routine into textual Intel-syntax assembler accepted by LLVM's integrated assembler.
     */
    template < typename It >
    inline std::string convert_to_x64_asm(It begin, It end, std::string const& name)
    {
        std::string result = ".text\n";
        result += ".intel_syntax noprefix\n";
        result += ".global " + name + "\n";
        result += ".type " + name + ", @function\n";
        result += name + ":\n";

        It pos = begin;
        while (pos != end)
        {
            asm_statement const statement = *pos++;
            if (typeis< asm_instruction >(statement))
            {
                asm_instruction const& inst = as< asm_instruction >(statement);
                result += "    " + to_lower_str(inst.opcode_mnemonic);
                for (std::size_t i = 0; i < inst.operands.size(); i++)
                {
                    result += (i == 0 ? " " : ", ");
                    result += inst.operands[i];
                }
                result += "\n";
                continue;
            }

            asm_label const& label = as< asm_label >(statement);
            result += label.name + ":\n";
        }

        return result;
    }
} // namespace quxlang

#endif // QUXLANG_BACKENDS_ASM_X64_ASM_CONVERTER_HEADER_GUARD
