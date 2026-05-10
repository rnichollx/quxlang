// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_ENUM_FLAGSET_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_ENUM_FLAGSET_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/context.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>

#include <optional>
#include <utility>

namespace quxlang::parsers
{
    std::vector< subdeclaroid > parse_subdeclaroids(parsing_context& ctx);

    /// Parses an optional BITS(width) clause following ENUM or FLAGSET.
    inline auto try_parse_nominal_bits_clause(parsing_context& ctx) -> std::optional< expression >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "BITS"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after BITS");
        }

        expression result = parse_expression(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after BITS expression");
        }
        return result;
    }

    /// Parses the optional associated declaration body, or requires the list-only trailing semicolon.
    inline auto parse_nominal_body_or_semicolon(parsing_context& ctx) -> std::vector< subdeclaroid >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, "{"))
        {
            std::vector< subdeclaroid > declarations = parse_subdeclaroids(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "}"))
            {
                throw syntax_compilation_error("Expected '}' after nominal type body");
            }
            return declarations;
        }

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' or associated declaration body after nominal type list");
        }
        return {};
    }

    /// Parses one ENUM value or RESERVED range entry.
    inline auto parse_enum_entry(parsing_context& ctx) -> ast2_enum_entry
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (skip_keyword_if_is(pos, end, "RESERVED"))
        {
            ast2_enum_reserved_range_declaration reserved;
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "FROM"))
            {
                throw syntax_compilation_error("Expected FROM in enum RESERVED range");
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after enum RESERVED FROM");
            }
            reserved.from = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after enum RESERVED FROM expression");
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "TO"))
            {
                throw syntax_compilation_error("Expected TO in enum RESERVED range");
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after enum RESERVED TO");
            }
            reserved.to = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after enum RESERVED TO expression");
            }
            reserved.location = ctx.get_location_optional(begin, pos);
            return reserved;
        }

        ast2_enum_value_declaration value;
        value.name = parse_identifier(pos, end);
        if (value.name.empty())
        {
            throw syntax_compilation_error("Expected enum item name");
        }

        bool parsed_equals = false;
        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            if (!value.is_default && skip_keyword_if_is(pos, end, "DEFAULT"))
            {
                value.is_default = true;
                continue;
            }
            if (!parsed_equals && skip_symbol_if_is(pos, end, "="))
            {
                parsed_equals = true;
                skip_whitespace_and_comments(pos, end);
                if (skip_keyword_if_is(pos, end, "NULL"))
                {
                    value.is_null = true;
                }
                else
                {
                    value.value = parse_expression(ctx);
                }
                continue;
            }
            break;
        }

        value.location = ctx.get_location_optional(begin, pos);
        return value;
    }

    /// Parses one FLAGSET value or RESERVED mask entry.
    inline auto parse_flagset_entry(parsing_context& ctx) -> ast2_flagset_entry
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (skip_keyword_if_is(pos, end, "RESERVED"))
        {
            ast2_flagset_reserved_declaration reserved;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "="))
            {
                throw syntax_compilation_error("Expected '=' after FLAGSET RESERVED");
            }
            reserved.mask = parse_expression(ctx);
            reserved.location = ctx.get_location_optional(begin, pos);
            return reserved;
        }

        ast2_flagset_value_declaration value;
        value.name = parse_identifier(pos, end);
        if (value.name.empty())
        {
            throw syntax_compilation_error("Expected flagset item name");
        }

        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, "="))
        {
            value.mask = parse_expression(ctx);
        }

        value.location = ctx.get_location_optional(begin, pos);
        return value;
    }

    /// Parses an ENUM declaration after its owning name.
    inline auto try_parse_enum_declaration(parsing_context& ctx) -> std::optional< ast2_enum_declaration >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "ENUM"))
        {
            return std::nullopt;
        }

        ast2_enum_declaration result;
        result.bit_width = try_parse_nominal_bits_clause(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "["))
        {
            throw syntax_compilation_error("Expected '[' after ENUM");
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "]"))
        {
            while (true)
            {
                result.entries.push_back(parse_enum_entry(ctx));
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "]"))
                {
                    break;
                }
                if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw syntax_compilation_error("Expected ',' or ']' in ENUM list");
                }
                skip_whitespace_and_comments(pos, end);
            }
        }

        skip_whitespace_and_comments(pos, end);
        result.allow_unknown = skip_keyword_if_is(pos, end, "ALLOW_UNKNOWN");
        result.declarations = parse_nominal_body_or_semicolon(ctx);
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }

    /// Parses a FLAGSET declaration after its owning name.
    inline auto try_parse_flagset_declaration(parsing_context& ctx) -> std::optional< ast2_flagset_declaration >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "FLAGSET"))
        {
            return std::nullopt;
        }

        ast2_flagset_declaration result;
        result.bit_width = try_parse_nominal_bits_clause(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "["))
        {
            throw syntax_compilation_error("Expected '[' after FLAGSET");
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "]"))
        {
            while (true)
            {
                result.entries.push_back(parse_flagset_entry(ctx));
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "]"))
                {
                    break;
                }
                if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw syntax_compilation_error("Expected ',' or ']' in FLAGSET list");
                }
                skip_whitespace_and_comments(pos, end);
            }
        }

        result.declarations = parse_nominal_body_or_semicolon(ctx);
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_TRY_PARSE_ENUM_FLAGSET_HEADER_GUARD
