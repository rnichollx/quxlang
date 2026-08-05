// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_EXTERN_TYPE_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_EXTERN_TYPE_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/string_literal.hpp>
#include <quxlang/parsers/symbol.hpp>

#include <optional>
#include <utility>

namespace quxlang::parsers
{
    /** Parses an EXTERN_TYPE declaration when present at the current position. */
    inline auto try_parse_extern_type_declaration(parsing_context& ctx) -> std::optional< ast2_extern_type >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "EXTERN_TYPE"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "["))
        {
            throw syntax_compilation_error("Expected '[' after EXTERN_TYPE");
        }

        skip_whitespace_and_comments(pos, end);
        std::optional< std::string > source = try_parse_string_literal(pos, end);
        if (!source.has_value())
        {
            throw syntax_compilation_error("Expected source name string literal in EXTERN_TYPE");
        }

        skip_whitespace_and_comments(pos, end);
        if (pos == end || *pos != ':')
        {
            throw syntax_compilation_error("Expected ':' after source name in EXTERN_TYPE");
        }
        ++pos;

        skip_whitespace_and_comments(pos, end);
        std::optional< std::string > external_name = try_parse_string_literal(pos, end);
        if (!external_name.has_value())
        {
            throw syntax_compilation_error("Expected external type name string literal in EXTERN_TYPE");
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "]"))
        {
            throw syntax_compilation_error("Expected ']' after EXTERN_TYPE external name");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' at end of EXTERN_TYPE declaration");
        }

        ast2_extern_type result{
            .source_name = std::move(*source),
            .external_type_name = std::move(*external_name),
        };
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_TRY_PARSE_EXTERN_TYPE_HEADER_GUARD
