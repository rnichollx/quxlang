// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_IDENTIFIER_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_IDENTIFIER_HEADER_GUARD
#include <string>

#include <quxlang/parsers/iter_parse_identifier.hpp>

namespace quxlang::parsers
{
    template < typename It >
    inline std::string parse_identifier(It& begin, It end)
    {
        auto pos = iter_parse_identifier(begin, end);
        if (pos == begin)
        {
            return {};
        }
        auto result = std::string(begin, pos);
        begin = pos;
        return result;
    }
}

#endif //GET_SKIP_IDENTIFIER_HPP
