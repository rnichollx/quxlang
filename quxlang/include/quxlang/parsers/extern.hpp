// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_EXTERN_HEADER_GUARD
#define QUXLANG_PARSERS_EXTERN_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include "parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/context.hpp"
#include "symbol.hpp"
#include "string_literal.hpp"
#include "quxlang/ast2/ast2_entity.hpp"

#include <utility>

namespace quxlang::parsers
{
    inline std::optional< ast2_extern > try_parse_ast2_extern(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "EXTERNAL"))
        {
            pos = begin;
            return std::nullopt;
        }

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after EXTERNAL");
        }

        skip_whitespace_and_comments(pos, end);

        auto lang = try_parse_string_literal(pos, end);

        if (!lang)
        {
            throw syntax_compilation_error("Expected language string after EXTERNAL(");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ","))
        {
            throw syntax_compilation_error("Expected ',' after EXTERNAL(\"" + std::string(*lang) + "\"");
        }

        skip_whitespace_and_comments(pos, end);

        auto name = try_parse_string_literal(pos, end);

        if (!name)
        {
            throw syntax_compilation_error("Expected name string after EXTERNAL(\"" + std::string(*lang) + "\",");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after EXTERNAL(\"" + std::string(*lang) + "\", \"" + std::string(*name) + "\"");
        }

        return ast2_extern{
            .lang = std::move(*lang),
            .symbol = std::move(*name)
        };
    }

}

#endif //EXTERN_HPP
