// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_DELEGATES_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_DELEGATES_HEADER_GUARD
#include <quxlang/parsers/try_parse_delegate_callsite_args.hpp>
#include <quxlang/parsers/try_parse_function_callsite_args.hpp>
#include <utility>

namespace quxlang::parsers
{
    inline std::vector< ast2_function_delegate > parse_function_delegates(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        std::vector< ast2_function_delegate > output;

        if (skip_symbol_if_is(pos, end, ":>"))
        {
        delegate:
            skip_whitespace_and_comments(pos, end);

            ast2_function_delegate d;
            d.target = parse_type_symbol(ctx);
            skip_whitespace_and_comments(pos, end);
            d.args = std::move(try_parse_delegate_callsite_args(ctx).value());
            skip_whitespace_and_comments(pos, end);
            output.push_back(std::move(d));
            if (skip_symbol_if_is(pos, end, ","))
            {
                goto delegate;
            }
        }
        return output;
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_DELEGATES_HPP
