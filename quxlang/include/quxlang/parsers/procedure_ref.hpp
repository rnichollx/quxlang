// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PROCEDURE_REF_HPP
#define QUXLANG_PROCEDURE_REF_HPP
#include "parse_type_symbol.hpp"
#include "parse_whitespace_and_comments.hpp"
#include "string_literal.hpp"
#include "quxlang/ast2/ast2_entity.hpp"

namespace quxlang::parsers
{
    template <typename It>
    std::optional< ast2_procedure_ref > try_parse_ast2_procedure_ref(It& it, It end)
    {
        // Example: PROCEDURE_REF( "ccall", foo::bar@(I32, MUT & I32) )
        auto pos = it;

        if (!skip_keyword_if_is(pos, end, "PROCEDURE_REF"))
        {
            return std::nullopt;
        }

        parsers::skip_whitespace_and_comments(pos, end);

        if (!parsers::skip_symbol_if_is(pos, end, "("))
        {
            throw std::runtime_error("Expected '(' after PROCEDURE_REF");
        }

        parsers::skip_whitespace_and_comments(pos, end);

        auto calling_convention = *try_parse_string_literal(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ","))
        {
            throw std::runtime_error("Expected ',' after PROCEDURE_REF(\"" + std::string(calling_convention) + "\"");
        }

        skip_whitespace_and_comments(pos, end);

        auto sym = parse_type_symbol(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::runtime_error("Expected ')' after PROCEDURE_REF(\"" + std::string(calling_convention) + "\", ...");
        }

        it = pos;

        return ast2_procedure_ref{
            .cc = calling_convention,
            .functanoid = sym
        };
    }
}

#endif //PROCEDURE_REF_HPP