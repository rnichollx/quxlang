//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_WHITESPACE_HPP
#define PARSE_WHITESPACE_HPP

#include <rylang/parsers/skip_whitespace.hpp>
#include <string>

namespace rylang::parsers
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

} // namespace rylang::parsers

#endif // PARSE_WHITESPACE_HPP
