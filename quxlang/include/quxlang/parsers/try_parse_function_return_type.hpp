// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_RETURN_TYPE_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_RETURN_TYPE_HEADER_GUARD

#include <optional>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>

namespace quxlang::parsers
{

    template < typename It >
    std::optional< type_symbol > try_parse_function_return_type(It& pos, It end)
    {
        std::optional< type_symbol > out;
        skip_whitespace(pos, end);

        if (!skip_symbol_if_is(pos, end, ":"))
        {
            return out;
        }
        skip_whitespace(pos, end);

        out = parse_type_symbol(pos, end);
        return out;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_RETURN_TYPE_HPP
