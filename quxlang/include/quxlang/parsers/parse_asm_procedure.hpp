// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_ASM_PROCEDURE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_ASM_PROCEDURE_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include "quxlang/ast2/ast2_entity.hpp"
#include <optional>
#include <utility>

#include "quxlang/parsers/asm/arm_assembler.hpp"
#include "quxlang/parsers/parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/parse_keyword.hpp"
#include "quxlang/parsers/asm/asm_callable.hpp"

namespace quxlang::parsers
{
    inline std::optional< ast2_asm_procedure_declaration > try_parse_asm_procedure_declaration(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

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
            throw syntax_compilation_error("Expected architecture name");
        }

        out.architecture = arch;

        skip_whitespace_and_comments(pos, end);

        std::optional< ast2_asm_callable > callable;

        loop:
        skip_whitespace_and_comments(pos, end);
        if ((callable = try_parse_asm_callable(ctx)).has_value())
        {
            out.callable_interfaces.push_back(std::move(*callable));
            skip_whitespace_and_comments(pos, end);
            goto loop;
        }
        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw syntax_compilation_error("Expected {");
        }

        if (arch == "ARM" || arch == "X64")
        {
            while (true)
            {
                auto next = try_parse_arm_instruction(ctx);
                if (!next.has_value())
                {
                    break;
                }
                out.instructions.push_back(std::move(*next));
            }
        }
        else
        {
            throw syntax_compilation_error("Unsupported architecture");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw syntax_compilation_error("Expected }");
        }
        
        out.location = ctx.get_location_optional(begin, pos);

        return std::move(out);
    }
} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_PARSE_ASM_PROCEDURE_HEADER
