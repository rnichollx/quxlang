// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_OBJECT_REF_HEADER_GUARD
#define QUXLANG_PARSERS_OBJECT_REF_HEADER_GUARD

#include "parse_type_symbol.hpp"
#include "parse_whitespace_and_comments.hpp"
#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/data/compilation_result.hpp"

#include <utility>

namespace quxlang::parsers
{
    /**
     * Parses an ASM OBJECT_REF operand component that lowers to the link name of a global object.
     */
    inline std::optional< ast2_object_ref > try_parse_ast2_object_ref(parsing_context& ctx)
    {
        // Example: OBJECT_REF(foo::bar)
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "OBJECT_REF"))
        {
            pos = begin;
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after OBJECT_REF");
        }

        skip_whitespace_and_comments(pos, end);

        type_symbol object = parse_type_symbol(ctx);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after OBJECT_REF(...)");
        }

        return ast2_object_ref{
            .object = std::move(object),
        };
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_OBJECT_REF_HEADER_GUARD
