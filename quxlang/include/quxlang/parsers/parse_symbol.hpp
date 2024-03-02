//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_SYMBOL_HPP
#define PARSE_SYMBOL_HPP
#include <quxlang/parsers/iter_parse_symbol.hpp>
#include <string>

namespace quxlang::parsers
{
    template < typename It >
    constexpr std::string parse_symbol(It & pos, It end)
    {
        auto sym_end = iter_parse_symbol(pos, end);
        std::string result = std::string(pos, sym_end);
        pos = sym_end;
        return result;
    }

    constexpr std::string parse_symbol(std::string input)
    {
        auto it = input.begin();
        auto it_e = input.end();
        return parse_symbol(it, input.end());
    }

    static_assert(parse_symbol("hello world") == "");
    static_assert(parse_symbol("%%a asdf") == "%%");
} // namespace quxlang

#endif // PARSE_SYMBOL_HPP
