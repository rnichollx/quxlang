// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_EXTERN_PROCEDURE_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_EXTERN_PROCEDURE_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/parsers/parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/parse_keyword.hpp"
#include "quxlang/parsers/string_literal.hpp"
#include "quxlang/parsers/symbol.hpp"
#include "quxlang/parsers/asm/asm_callable.hpp"

#include <optional>
#include <utility>

namespace quxlang::parsers
{
    inline std::optional< ast2_extern_procedure > try_parse_extern_procedure_declaration(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "EXTERN_PROCEDURE"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "["))
        {
            throw syntax_compilation_error("Expected '[' after EXTERN_PROCEDURE");
        }

        skip_whitespace_and_comments(pos, end);

        auto library_opt = try_parse_string_literal(pos, end);
        if (!library_opt)
        {
            throw syntax_compilation_error("Expected library name string literal in EXTERN_PROCEDURE");
        }

        skip_whitespace_and_comments(pos, end);

        if (pos == end || *pos != ':')
        {
            throw syntax_compilation_error("Expected ':' after library name in EXTERN_PROCEDURE");
        }
        ++pos;

        skip_whitespace_and_comments(pos, end);

        auto symbol_opt = try_parse_string_literal(pos, end);
        if (!symbol_opt)
        {
            throw syntax_compilation_error("Expected external symbol name string literal in EXTERN_PROCEDURE");
        }

        std::optional< std::string > version;
        bool is_optional = false;

        while (true)
        {
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "]"))
            {
                break;
            }

            if (skip_keyword_if_is(pos, end, "VERSION"))
            {
                skip_whitespace_and_comments(pos, end);
                auto version_opt = try_parse_string_literal(pos, end);
                if (!version_opt)
                {
                    throw syntax_compilation_error("Expected version string literal after VERSION in EXTERN_PROCEDURE");
                }
                version = std::move(*version_opt);
            }
            else if (skip_keyword_if_is(pos, end, "OPTIONAL"))
            {
                is_optional = true;
            }
            else
            {
                throw syntax_compilation_error("Unexpected token or keyword in EXTERN_PROCEDURE configuration block");
            }
        }

        skip_whitespace_and_comments(pos, end);

        auto callable = try_parse_asm_procedure_callable(ctx);

        if (!callable.has_value())
        {
            throw syntax_compilation_error("Expected CALLABLE block in EXTERN_PROCEDURE");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' at end of EXTERN_PROCEDURE declaration");
        }

        ast2_extern_procedure out;
        out.library_name = std::move(*library_opt);
        out.external_symbol_name = std::move(*symbol_opt);
        out.version = std::move(version);
        out.is_optional = is_optional;
        out.callable = std::move(callable);
        out.location = ctx.get_location_optional(begin, pos);

        return out;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_TRY_PARSE_EXTERN_PROCEDURE_HEADER_GUARD
