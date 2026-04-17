// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_TYPE_SYMBOL_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_TYPE_SYMBOL_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>
#include <optional>
#include <utility>

namespace quxlang::parsers
{
    std::optional< type_symbol > try_parse_type_symbol(parsing_context& ctx);

    inline type_symbol parse_type_symbol(parsing_context& ctx)
    {
        auto result = try_parse_type_symbol(ctx);
        if (!result)
        {
            throw std::logic_error("Expected type symbol");
        }
        return std::move(*result);
    }

    inline argif parse_argif(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        argif result;
        result.type = parse_type_symbol(ctx);
        skip_whitespace(pos, end);
        if (skip_keyword_if_is(pos, end, "DEFAULTED"))
        {
            result.is_defaulted = true;
        }
        else
        {
            result.is_defaulted = false;
        }
        return result;
    }
} // namespace quxlang::parsers

#endif // PARSE_TYPE_SYMBOL_HPP
