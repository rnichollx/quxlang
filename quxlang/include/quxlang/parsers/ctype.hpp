// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_CTYPE_HEADER_GUARD
#define QUXLANG_PARSERS_CTYPE_HEADER_GUARD


namespace quxlang::parsers
{
    inline constexpr bool is_space(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    inline constexpr bool is_alpha(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    inline constexpr bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    inline constexpr bool is_alnum(char c)
    {
        return is_alpha(c) || is_digit(c);
    }


}

#endif //CTYPE_HPP
