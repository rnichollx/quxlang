//
// Created by Ryan Nicholl on 2/24/24.
//

#ifndef RPNX_QUXLANG_PARSE_ASM_PROCEDURE_HEADER
#define RPNX_QUXLANG_PARSE_ASM_PROCEDURE_HEADER

#include "rylang/ast2/ast2_entity.hpp"
#include <optional>

#include "rylang/parsers/asm/arm_assembler.hpp"
#include "rylang/parsers/parse_whitespace_and_comments.hpp"
#include "rylang/parsers/parse_keyword.hpp"

namespace rylang::parsers
{
    template < typename It >
    std::optional< ast2_asm_procedure_declaration > try_parse_asm_procedure_declaration(It& begin, It end)
    {
        auto pos = begin;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "ASM_PROCEDURE"))
        {
            return std::nullopt;
        }

        ast2_asm_procedure_declaration out;

        skip_whitespace_and_comments(pos, end);

        std::string arch = parse_keyword(pos, end);

        if (arch.empty())
        {
            throw std::runtime_error("Expected architecture name");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::runtime_error("Expected {");
        }

        if (arch == "ARM")
        {
            while (true)
            {
                auto next = try_parse_arm_instruction(pos, end);
                if (!next.has_value())
                {
                    break;
                }
                out.instructions.push_back(next.value());
            }
        }
        else
        {
            throw std::runtime_error("Unsupported architecture");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::runtime_error("Expected }");
        }
        
        begin = pos;

        return out;
    }
} // namespace rylang::parsers

#endif // RPNX_QUXLANG_PARSE_ASM_PROCEDURE_HEADER
