// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BACKENDS_ASM_GNU_ASM_CONVERTER_HEADER_GUARD
#define QUXLANG_BACKENDS_ASM_GNU_ASM_CONVERTER_HEADER_GUARD

#include "quxlang/asm/asm.hpp"
#include "quxlang/backends/asm/symbol_format.hpp"
#include "quxlang/variant_utils.hpp"
#include "rpnx/unimplemented.hpp"

#include <string>

namespace quxlang
{
    inline std::string to_lower_str(std::string str)
    {
        for (char& i : str)
        {
            if (i >= 'A' && i <= 'Z')
            {
                i = i - 'A' + 'a';
            }
        }
        return str;
    }

    /**
     * Converts an assembly routine using GNU operand syntax into textual assembly accepted by LLVM's integrated assembler.
     */
    template < typename It >
    inline std::string convert_to_gnu_asm(It begin, It end, std::string const& name, bool emit_elf_type = false)
    {
        std::string const asm_name = format_asm_symbol_name(name);
        std::string result = ".text\n";
        result += ".global " + asm_name + "\n";
        if (emit_elf_type)
        {
            result += ".type " + asm_name + ", @function\n";
        }
        result += asm_name + ":\n";

        It pos = begin;
        while (pos != end)
        {
            asm_statement const statement = *pos++;
            if (typeis< asm_instruction >(statement))
            {
                asm_instruction const& instruction = as< asm_instruction >(statement);
                result += "    " + to_lower_str(instruction.opcode_mnemonic);
                for (std::size_t i = 0; i < instruction.operands.size(); i++)
                {
                    result += (i == 0 ? " " : ", ");
                    result += instruction.operands[i];
                }
                result += "\n";
                continue;
            }

            if (typeis< asm_label >(statement))
            {
                asm_label const& label = as< asm_label >(statement);
                result += label.name + ":\n";
                continue;
            }

            rpnx::unimplemented();
        }
        return result;
    }
} // namespace quxlang

#endif // QUXLANG_BACKENDS_ASM_GNU_ASM_CONVERTER_HEADER_GUARD
