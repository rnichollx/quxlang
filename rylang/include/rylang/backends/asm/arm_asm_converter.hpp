//
// Created by Ryan Nicholl on 3/1/24.
//

#ifndef RPNX_QUXLANG_ARM_ASM_CONVERTER_HEADER
#define RPNX_QUXLANG_ARM_ASM_CONVERTER_HEADER

#include "rylang/asm/asm.hpp"
#include <string>

namespace rylang
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

    template < typename It >
    inline std::string convert_to_arm_asm(It begin, It end, std::string name)
    {
        std::string result = ".text\n";
        result += ".global " + name + "\n";

        result += name + ":\n";
        It pos = begin;

        while (pos != end)
        {
            asm_instruction inst = *pos++;

            std::string opcode_str = inst.opcode_mnemonic;
            opcode_str = to_lower_str(opcode_str);

            result += "    " + opcode_str;
            for (std::size_t i = 0; i < inst.operands.size(); i++)
            {
                if (i != 0)
                {
                    result += ", ";
                }
                else
                {
                    result += " ";
                }

                result += inst.operands[i];
            }

            result += "\n";
        }
        return result;
    }
} // namespace rylang


#endif // RPNX_QUXLANG_ARM_ASM_CONVERTER_HEADER
