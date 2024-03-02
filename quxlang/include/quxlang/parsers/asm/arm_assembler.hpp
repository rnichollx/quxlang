//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef RPNX_QUXLANG_ARM_ASSEMBLER_HEADER
#define RPNX_QUXLANG_ARM_ASSEMBLER_HEADER

#include "quxlang/asm/asm.hpp"

#include <iostream>
#include <optional>
#include <set>
#include <string>

#include "quxlang/parsers/parse_whitespace_and_comments.hpp"

namespace quxlang::parsers
{

    template < typename It >
    inline std::optional< asm_instruction > try_parse_arm_instruction(It& in_pos, It end)
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

        asm_instruction ret;

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

            std::string operand;

            while (pos != end && (*pos != ',' || bracket_count == 0) && *pos != ';' && *pos != '\n' && *pos != '\r' && *pos != '\t' && *pos != '}')
            {
                operand.push_back(*pos);
                if (*pos == '[')
                {
                    bracket_count++;
                }
                else if (*pos == ']')
                {
                    if (bracket_count == 0)
                    {
                      throw std::runtime_error("Mismatched brackets");
                    }
                    bracket_count--;
                }
                ++pos;
            }

            ret.operands.push_back(operand);

            if (skip_symbol_if_is(pos, end, ";"))
            {
                break;
            }

            if (!skip_symbol_if_is(pos, end, ","))
            {

                throw std::runtime_error("expected , or ;");
            }
        }

        in_pos = pos;
        return ret;

    } // namespace quxlang::parsers

} // namespace quxlang::parsers
#endif // RPNX_QUXLANG_ARM_ASSEMBLER_HEADER
