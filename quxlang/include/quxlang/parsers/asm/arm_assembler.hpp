//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef QUXLANG_PARSERS_ASM_ARM_ASSEMBLER_HEADER_GUARD
#define QUXLANG_PARSERS_ASM_ARM_ASSEMBLER_HEADER_GUARD

#include "../../../../../rpnx/include/rpnx/debug.hpp"
#include "quxlang/asm/asm.hpp"
#include "quxlang/parsers/extern.hpp"

#include <iostream>
#include <optional>
#include <set>
#include <string>

#include "quxlang/parsers/parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/procedure_ref.hpp"
#include "quxlang/parsers/symbol.hpp"

namespace quxlang::parsers
{

    template <typename It>
    std::optional< ast2_asm_operand_component > try_parse_arm_asm_operand_component(It& pos, It end)
    {
        QUXLANG_DEBUG({std::cout << "try_parse_arm_asm_operand_component" << std::endl;});
        skip_whitespace_and_comments(pos, end);
        std::optional< ast2_extern > exte = try_parse_ast2_extern(pos, end);
        if (exte)
        {
            return *exte;
        }

        std::optional< ast2_procedure_ref > proc = try_parse_ast2_procedure_ref(pos, end);
        if (proc)
        {
            return *proc;
        }

        std::string outstr;

        while (pos != end &&

               // We need to exclude "EXTERNAL" and "PROCEDURE" because they need to be
               // replaced with their linker symbol during conversion from AST2_ASM_INSTRUCTION
               // to ASM_INSTRUCTION
               next_keyword(pos, end) != "EXTERNAL" &&
               next_keyword(pos, end) != "PROCEDURE" &&

               // We also need to exclude the comma, because it is a separator between components,
               // but only sometimes. However, the case where it is part of an operand is handled
               // by the try_parse_arm_asm_operand function.
               *pos != ',' &&

               // ';' terminates the operand in Qux assembly syntax
               *pos != ';' &&

               // These are handled by the try_parse_arm_asm_operand function, not here
               *pos != '[' && *pos != ']' &&

               // No multi-line statements in Qux assembly syntax
               *pos != '\n' && *pos != '\r' && *pos != '\t' &&

               // Shouldn't encounter this, this indicates the end of the assembler input
               *pos != '}'
        )
        {
            outstr.push_back(*pos);
            ++pos;
        }

        if (outstr.empty())
        {
            return std::nullopt;
        }

        return outstr;
    }


    template <typename It>
    inline std::optional< ast2_asm_operand > try_parse_arm_asm_operand(It& it, It end)
    {

        QUXLANG_DEBUG({std::cout << "try_parse_arm_asm_operand" << std::endl;});
        ast2_asm_operand ret;
        std::size_t bracket_count = 0;

    get_part:

        std::cout << "get_part" << std::endl;
        skip_whitespace_and_comments(it, end);

        if (skip_symbol_if_is(it, end, "["))
        {
            std::cout << "bracket_count++" << std::endl;
            bracket_count++;
            ret.components.push_back(std::string("["));
            goto get_part;
        }
        else if (skip_symbol_if_is(it, end, "]"))
        {
            if (bracket_count == 0)
            {
                throw std::logic_error("Mismatched brackets");
            }
            bracket_count--;
            ret.components.push_back(std::string("]"));
            goto get_part;
        }

        else if (bracket_count != 0 && skip_symbol_if_is(it, end, ","))
        {
            ret.components.push_back(std::string(","));
            goto get_part;
        }

        auto comp = try_parse_arm_asm_operand_component(it, end);

        if (comp)
        {
            QUXLANG_DEBUG({
                if (typeis< std::string >(*comp))
                {
                std::cout << "comp: " << as< std::string >(*comp) << std::endl;
                }
                else if (typeis< ast2_extern >(*comp))
                {
                std::cout << "comp: EXTERNAL" << std::endl;
                }
                else if (typeis< ast2_procedure_ref >(*comp))
                {
                std::cout << "comp: PROCEDURE" << std::endl;
                }
                else
                {
                std::cout << "comp: err?" << std::endl;
                }
                });
            ret.components.push_back(*comp);
            goto get_part;
        }

        return ret;

    }


    template <typename It>
    inline std::optional< ast2_asm_instruction > try_parse_arm_instruction(It& in_pos, It end)
    {
        std::set< std::string > arm_instruction_opcodes{"ADC", "ADCS", "ADD", "ADDG", "ADDS", "ADR", "ADRP", "AND", "ANDS", "ASR", "ASRV", "AT", "B", "BFI", "BFC", "BFXIL", "BIC", "BICS", "BL", "BLR", "BR", "BRK", "BSL", "CBNZ", "CBZ", "CCMN", "CCMP", "CINC", "CINV", "CLREX", "CLS", "CLZ", "CMN", "CMP", "CNEG", "CRC32B", "CRC32CB", "CRC32CH", "CRC32CW", "CRC32CX", "CRC32H", "CRC32W", "CRC32X", "CSEL", "CSINC", "CSINV", "CSNEG", "DC", "DCPS1", "DCPS2", "DCPS3", "DMB", "DRPS", "DSB", "DUP", "EON", "EOR", "ERET", "EXTR", "HINT", "HLT", "HVC", "IC", "ISB", "LD1", "LD1R", "LD2", "LD2R", "LD3", "LD3R", "LD4", "LD4R", "LDADD", "LDADDB", "LDADDH", "LDADDL", "LDADDLB", "LDADDLH", "LDAXP", "LDAXR", "LDCLR", "LDCLRB", "LDCLRH", "LDCLRL", "LDCLRLB", "LDCLRLH", "LDEOR", "LDEORB", "LDEORH", "LDEORL", "LDEORLB", "LDEORLH", "LDLAR", "LDLARB", "LDLARH", "LDNP", "LDP", "LDPSW", "LDR", "LDRAA", "LDRAB", "LDRB", "LDRH", "LDRSB", "LDRSH", "LDRSW", "LDSET", "LDSETB", "LDSETH", "LDSETL", "LDSETLB", "LDSETLH", "LDSMAX", "LDSMAXB", "LDSMAXH", "LDSMAXL", "LDSMAXLB", "LDSMAXLH", "LDSMIN", "LDSMINB", "LDSMINH", "LDSMINL", "LDSMINLB", "LDSMINLH", "LDTR", "LDTRB", "LDTRH", "LDTRSB", "LDTRSH", "LDTRSW", "LDUMAX", "LDUMAXB", "LDUMAXH", "LDUMAXL", "LDUMAXLB", "LDUMAXLH", "LDUMIN", "LDUMINB", "LDUMINH", "LDUMINL", "LDUMINLB", "LDUMINLH", "LDUR", "LDURB", "LDURH", "LDURSB", "LDURSH", "LDURSW", "LDXP", "LDXR", "LSL", "LSLV", "LSR", "LSRV", "MADD", "MLA", "MLS", "MOV", "MOVK", "MOVN", "MOVZ", "MRS", "MSR", "MUL", "MVN", "NEG", "NEGS", "NGC", "NGCS", "NOP", "ORN", "ORR", "PACDA", "PACDB", "PACDZA", "PACDZB", "PACGA", "PACIA", "PACIA1716", "PACIAA", "PACIASP", "PACIAZ", "PACIB", "PACIB1716", "PACIBA", "PACIBSP", "PACIBZ", "PACIZA", "PACIZB", "PRFM", "PRFUM", "PSB", "RBIT", "RET", "RETAA", "RETAB", "REV", "REV16", "REV32", "REV64", "ROR", "RORV", "SBC", "SBCS", "SBFIZ", "SBFM", "SBI", "SBIX", "SCVTF", "SDIV", "SEV", "SEVL", "SMADDL", "SMC", "SMULH", "SMULL", "SMSUBL", "SPLICE", "SQADD", "SQDMLAL", "SQDMLAL2", "SQDMLSL", "SQDMLSL2", "SQDMULH", "SQDMULL", "SQDMULL2", "SQNEG", "SQRDMULH", "SQRSHL", "SQRSHRN", "SQRSHRN2", "SQRSHRUN", "SQRSHRUN2", "SQSHL", "SQSHLU", "SQSHRN", "SQSHRN2", "SQSHRUN", "SQSHRUN2", "SQSUB", "SQXTN", "SQXTN2", "SQXTUN", "SQXTUN2", "SRHADD", "SRI", "SRSHR", "SRSRA", "SSBB", "SSHL", "SSHLL", "SSHLL2", "SSHR", "SSRA", "SSUBL", "SSUBL2", "SUQADD", "SVC", "SWP", "SWPA", "SWPAB", "SWPAL", "SWPALB", "SWPALH", "SWPAX", "SWPB", "SWPH", "SWPL", "SWPLB", "SWPLH", "SWPX", "SXTB", "SXTH", "SXTL", "SXTL2", "SXTW", "SYS", "SYSL", "TBNZ", "TBZ", "TLBI", "TRN1", "TRN2", "TST", "UABA", "UABAL", "UABAL2", "UABD", "UABDL", "UABDL2", "UADDL", "UADDL2", "UADDLP", "UADDLV", "UADDW", "UADDW2", "UBFIZ", "UBFM", "UBFX", "UDF", "UDIV", "UHADD", "UHSUB", "UMADDL", "UMAX", "UMAXP", "UMAXV", "UMIN", "UMINP", "UMINV", "UMLAL", "UMLAL2", "UMLSL", "UMLSL2", "UMOV", "UMSUBL", "UMULH", "UMULL", "UMULL2", "UQADD", "UQRSHL", "UQRSHRN", "UQRSHRN2", "UQSHL", "UQSHRN", "UQSHRN2", "UQSUB", "UQXTN", "UQXTN2", "URECPE", "URHADD", "URSHL", "URSHR", "URSQRTE", "URSRA", "USADALP", "USDOT", "USDOT", "USHL", "USHL", "USHLL", "USHLL2", "USHR", "USQADD", "USRA", "USUBL", "USUBL2", "UXTB", "UXTH", "UXTL", "UXTL2", "UZP1", "UZP2", "WFE", "WFI", "XPACD", "XPACI", "XPACLRI", "YIELD", "ZIP1", "ZIP2"};

        auto pos = in_pos;

        skip_whitespace_and_comments(pos, end);

        std::string kw = parse_keyword(pos, end);

        if (kw == "")
        {
            return std::nullopt;
        }

        if (arm_instruction_opcodes.find(kw) == arm_instruction_opcodes.end())
        {
            // For now, no error checking, because the list of opcodes might not be complete
            std::cout << "Warning: opcode '" << kw << "' not recognized" << std::endl;
            // return std::nullopt;
        }

        ast2_asm_instruction ret;

        ret.opcode_mnemonic = kw;

        int bracket_count = 0;
        while (pos != end)
        {
            auto pstart = pos;
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ";"))
            {
                break;
            }

            auto operand = try_parse_arm_asm_operand(pos, end);
            if (!operand)
            {
                throw std::logic_error("expected operand");
            }

            ret.operands.push_back(*operand);

            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, ";"))
            {
                break;
            }

            if (!skip_symbol_if_is(pos, end, ","))
            {

                throw std::logic_error("expected , or ;");
            }
        }

        in_pos = pos;
        return ret;

    } // namespace quxlang::parsers

} // namespace quxlang::parsers
#endif // RPNX_QUXLANG_ARM_ASSEMBLER_HEADER