// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef SYMBOL_HPP
#define SYMBOL_HPP
#include <string>

#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
{

    template <typename It>
    constexpr auto iter_parse_symbol(It begin, It end) -> It
    {
        auto pos = begin;
        bool started = false;
        while (pos != end && !is_space(*pos) && !is_alpha(*pos) && !is_digit(*pos) && (!started || ((*pos != ')') && (*pos != '{' && *pos != '}') && (*pos != ',' && *pos != ';'))) && *pos != '_')
        {
            char c = *pos;
            ++pos;

            if (c == '(' || c == ')')
            {
                break;
            }
            started = true;
        }

        return pos;
    }

    template <typename It>
    constexpr std::string parse_symbol(It& pos, It end)
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
}
#endif //SYMBOL_HPP