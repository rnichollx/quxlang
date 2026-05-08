// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_ASM_ARM_ASSEMBLER_HEADER_GUARD
#define QUXLANG_PARSERS_ASM_ARM_ASSEMBLER_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"


#include "quxlang/asm/asm.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/parsers/extern.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "quxlang/parsers/parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/procedure_ref.hpp"
#include "quxlang/parsers/symbol.hpp"

namespace quxlang::parsers
{

    inline std::optional< ast2_asm_operand_component > try_parse_arm_asm_operand_component(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        skip_whitespace_and_comments(pos, end);
        std::optional< ast2_extern > exte = try_parse_ast2_extern(ctx);
        if (exte)
        {
            return std::move(*exte);
        }

        std::optional< ast2_procedure_ref > proc = try_parse_ast2_procedure_ref(ctx);
        if (proc)
        {
            return std::move(*proc);
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

               // ';' terminates the operand in Quxlang assembly syntax
               *pos != ';' &&

               // These are handled by the try_parse_arm_asm_operand function, not here
               *pos != '[' && *pos != ']' &&

               // No multi-line statements in Quxlang assembly syntax
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


    inline std::optional< ast2_asm_operand > try_parse_arm_asm_operand(parsing_context& ctx)
    {
        auto& it = ctx.iter_pos;
        auto end = ctx.iter_end;

      //  QUXLANG_DEBUG({std::cout << "try_parse_arm_asm_operand" << std::endl;});
        ast2_asm_operand ret;
        std::size_t bracket_count = 0;

    get_part:

        skip_whitespace_and_comments(it, end);

        if (skip_symbol_if_is(it, end, "["))
        {
            bracket_count++;
            ret.components.push_back(std::string("["));
            goto get_part;
        }
        else if (skip_symbol_if_is(it, end, "]"))
        {
            if (bracket_count == 0)
            {
                throw syntax_compilation_error("Mismatched brackets");
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

        auto comp = try_parse_arm_asm_operand_component(ctx);

        if (comp)
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
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
            }
            ret.components.push_back(std::move(*comp));
            goto get_part;
        }

        return ret;

    }


    inline std::optional< ast2_asm_instruction > try_parse_arm_instruction(parsing_context& ctx)
    {
        auto trial = ctx;
        auto& pos = trial.iter_pos;
        auto end = trial.iter_end;

        skip_whitespace_and_comments(pos, end);

        std::string kw = parse_keyword(pos, end);

        if (kw == "")
        {
            return std::nullopt;
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

            auto operand = try_parse_arm_asm_operand(trial);
            if (!operand)
            {
                throw syntax_compilation_error("expected operand");
            }

            ret.operands.push_back(std::move(*operand));

            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, ";"))
            {
                break;
            }

            if (!skip_symbol_if_is(pos, end, ","))
            {

                throw syntax_compilation_error("expected , or ;");
            }
        }

        ctx.iter_pos = pos;
        return ret;

    } // namespace quxlang::parsers

} // namespace quxlang::parsers
#endif // RPNX_QUXLANG_ARM_ASSEMBLER_HEADER
