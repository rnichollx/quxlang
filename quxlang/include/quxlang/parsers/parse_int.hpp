// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_INT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_INT_HEADER_GUARD
#include <quxlang/parsers/ctype.hpp>
#include <string>

namespace quxlang::parsers
{
    template < typename It >
    constexpr std::string parse_int(It& pos, It end)
    {
        std::string out;
        while (pos != end && is_digit(*pos))
        {
            out += *pos;
            ++pos;
        }
        return out;
    }

    template < typename Int >
    constexpr Int str_to_int(const std::string& str)
    {
        static_assert(std::is_integral_v< Int >, "Template parameter must be an integral type");

        Int result = 0;
        std::size_t i = 0;
        bool is_negative = false;

        // Skip leading whitespace
        while (i < str.size() && std::isspace(static_cast< unsigned char >(str[i])))
        {
            ++i;
        }

        // Handle optional sign
        if (i < str.size() && (str[i] == '+' || str[i] == '-'))
        {
            is_negative = str[i] == '-';
            ++i;
        }

        if (i == str.size() || !std::isdigit(static_cast< unsigned char >(str[i])))
        {
            throw std::invalid_argument("Invalid integer string: " + str);
        }

        for (; i < str.size(); ++i)
        {
            char c = str[i];
            if (!std::isdigit(static_cast< unsigned char >(c)))
            {
                throw std::invalid_argument("Invalid character in input: " + str);
            }

            int digit = c - '0';

            if (result > (std::numeric_limits< Int >::max() - digit) / 10)
            {
                throw std::out_of_range("Integer overflow in input: " + str);
            }

            result = result * 10 + digit;
        }

        return is_negative ? -result : result;
    }
} // namespace quxlang::parsers

#endif // PARSE_INT_HPP
