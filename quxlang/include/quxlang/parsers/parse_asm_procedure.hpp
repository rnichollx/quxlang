// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_ASM_PROCEDURE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_ASM_PROCEDURE_HEADER_GUARD

#include "linkname.hpp"
#include "quxlang/ast2/ast2_entity.hpp"
#include <optional>

#include "quxlang/parsers/asm/arm_assembler.hpp"
#include "quxlang/parsers/parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/parse_keyword.hpp"
#include "quxlang/parsers/asm/asm_callable.hpp"

namespace quxlang::parsers
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
            throw std::logic_error("Expected architecture name");
        }

        skip_whitespace_and_comments(pos, end);

        std::optional< ast2_asm_callable > callable;

        loop:
        skip_whitespace_and_comments(pos, end);
        if ((callable = try_parse_asm_callable(pos, end)).has_value())
        {
            out.callable_interfaces.push_back(callable.value());
            skip_whitespace_and_comments(pos, end);
            goto loop;
        }
        else if (auto linkname = try_parse_linkname(pos, end); linkname.has_value())
        {
            out.linkname = linkname;
            skip_whitespace_and_comments(pos, end);
            goto loop;
        }



        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::logic_error("Expected {");
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
            throw std::logic_error("Unsupported architecture");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw std::logic_error("Expected }");
        }
        
        begin = pos;

        return out;
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_PARSE_ASM_PROCEDURE_HEADER
