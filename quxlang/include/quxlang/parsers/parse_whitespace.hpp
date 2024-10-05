// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_WHITESPACE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_WHITESPACE_HEADER_GUARD

#include <quxlang/parsers/skip_whitespace.hpp>
#include <string>

namespace quxlang::parsers
{
    template < typename It >
    constexpr std::string parse_whitespace(It& pos, It end)
    {
        auto it = pos;
        skip_whitespace(it, end);
        std::string result = std::string(pos, it);
        pos = it;
        return result;
    }

    constexpr std::string parse_whitespace(std::string input)
    {
        auto it = input.begin();
        auto it_e = input.end();
        return parse_whitespace(it, it_e);
    }

    static_assert(parse_whitespace("hello world") == "");
    static_assert(parse_whitespace("   a asdf") == "   ");

} // namespace quxlang::parsers

#endif // PARSE_WHITESPACE_HPP
