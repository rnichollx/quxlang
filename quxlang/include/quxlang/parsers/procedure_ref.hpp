// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_PROCEDURE_REF_HEADER_GUARD
#define QUXLANG_PARSERS_PROCEDURE_REF_HEADER_GUARD
#include "parse_type_symbol.hpp"
#include "parse_whitespace_and_comments.hpp"
#include "string_literal.hpp"
#include "quxlang/ast2/ast2_entity.hpp"

namespace quxlang::parsers
{
    inline std::optional< ast2_procedure_ref > try_parse_ast2_procedure_ref(parsing_context& ctx)
    {
        // Example: PROCEDURE_REF( "ccall", foo::bar#(I32, MUT & I32) )
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "PROCEDURE_REF"))
        {
            pos = begin;
            return std::nullopt;
        }

        parsers::skip_whitespace_and_comments(pos, end);

        if (!parsers::skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '(' after PROCEDURE_REF");
        }

        parsers::skip_whitespace_and_comments(pos, end);

        auto calling_convention = *try_parse_string_literal(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ","))
        {
            throw std::logic_error("Expected ',' after PROCEDURE_REF(\"" + std::string(calling_convention) + "\"");
        }

        skip_whitespace_and_comments(pos, end);

        auto sym = parse_type_symbol(ctx);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::logic_error("Expected ')' after PROCEDURE_REF(\"" + std::string(calling_convention) + "\", ...");
        }

        return ast2_procedure_ref{
            .cc = calling_convention,
            .functanoid = sym
        };
    }
}

#endif //PROCEDURE_REF_HPP
