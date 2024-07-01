// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUX_IMPORT_HPP
#define QUX_IMPORT_HPP


#include "parse_type_symbol.hpp"
#include "parse_whitespace_and_comments.hpp"
#include "symbol.hpp"

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/parsers/keyword.hpp>


namespace quxlang::parsers
{
    template <typename It>
    std::optional< type_symbol > try_parse_import(It& it, It end)
    {
        auto pos = it;
        skip_whitespace_and_comments(it, end);

        if (!skip_keyword_if_is(pos, end, "IMPORT"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '(' after IMPORT");
        }

        type_symbol import_symbol = parse_type_symbol(pos, end);

    }

}

#endif //IMPORT_HPP