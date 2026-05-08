// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_PARSERS_SYMBOL_HEADER_GUARD
#define QUXLANG_PARSERS_SYMBOL_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
{

    template < typename It >
    constexpr auto iter_parse_symbol(It begin, It end) -> It
    {
        auto pos = begin;
        bool started = false;
        char c;
        while (pos != end && (c = *pos, !is_space(c)) && !is_alpha(c) && !is_digit(c) && (!started || ((c != ')') && ( c != '}') && (c != ',' && c != ';'))) && c != '_')
        {
            ++pos;
            if (c == '(' || c == ')')
            {
                break;
            }
            if (c == '{' || c == '}')
            {
                break;
            }
            if (c == '[' && (pos == end || *pos != '&'))
            {
                break;
            }

            if (c == ']')
            {
                break;
            }

            started = true;
        }

        return pos;
    }

    template < typename It >
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

    template < typename It >
    inline bool skip_symbol_if_is(It& begin, It end, std::string_view symbol)
    {
        auto pos = iter_parse_symbol(begin, end);
        if (pos == begin)
        {
            return false;
        }
        if (static_cast< std::size_t >(std::distance(begin, pos)) == symbol.size() && std::equal(begin, pos, symbol.begin(), symbol.end()))
        {
            begin = pos;
            return true;
        }
        return false;
    }

    template < typename It >
    inline void consume_symbol(It& begin, It end, std::string_view symbol)
    {
        auto pos = iter_parse_symbol(begin, end);
        if (pos == begin)
        {
            throw syntax_compilation_error("Expected symbol");
        }
        if (static_cast< std::size_t >(std::distance(begin, pos)) != symbol.size() || !std::equal(begin, pos, symbol.begin(), symbol.end()))
        {
            throw syntax_compilation_error("Expected symbol");
        }
        begin = pos;
    }

} // namespace quxlang::parsers
#endif // SYMBOL_HPP
