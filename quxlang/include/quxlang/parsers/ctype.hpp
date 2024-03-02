//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef QUXLANG_PARSERS_CTYPE_HEADER_GUARD
#define QUXLANG_PARSERS_CTYPE_HEADER_GUARD


namespace quxlang::parsers
{
    constexpr bool is_space(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    constexpr bool is_alpha(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    constexpr bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    constexpr bool is_alnum(char c)
    {
        return is_alpha(c) || is_digit(c);
    }

    template <typename It>
    constexpr std::optional<int> str_to_i(It str, It end)
    {
        int out = 0;
        int sign = 1;

        if (*str == '-')
        {
            sign = -1;
            str++;
        }

        while (str != end)
        {
            if (!is_digit(*str)) return std::nullopt;
            out *= 10;
            out += *str - '0';
            str++;
        }

        return out * sign;
    }
}

#endif //CTYPE_HPP
