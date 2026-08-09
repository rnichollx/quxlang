// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_DELEGATE_CALLSITE_ARGS_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_DELEGATE_CALLSITE_ARGS_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <quxlang/data/basic_types.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>

namespace quxlang::parsers
{
    /** Parses either mixed constructor arguments or the positional-only constructor shorthand for a delegate. */
    inline auto try_parse_delegate_callsite_args(parsing_context& ctx) -> std::optional< std::vector< expression_arg > >
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ":("))
        {
            return parse_call_argument_list(ctx, ")");
        }
        if (skip_symbol_if_is(pos, end, ":["))
        {
            return parse_positional_argument_sequence(ctx, "]");
        }
        return std::nullopt;
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_DELEGATE_CALLSITE_ARGS_HPP
