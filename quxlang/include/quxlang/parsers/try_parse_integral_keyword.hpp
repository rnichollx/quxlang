// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_INTEGRAL_KEYWORD_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_INTEGRAL_KEYWORD_HEADER_GUARD

#include <optional>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
{
    template < typename It >
    constexpr std::optional< int_type > try_parse_integral_keyword(It& pos, It end)
    {
        int_type ast{};
        auto it = pos;

        if (it != end && (*it == 'I' || *it == 'U'))
        {
            ast.has_sign = *it++ == 'I';

            auto dig_start = it;
            while (it != end && is_digit(*it))
            {
                ++it;
            }

            std::string dig_str = std::string(dig_start, it);
            if (dig_str.empty())
            {
                return std::nullopt;
            }

            ast.bits = str_to_i(dig_str.begin(), dig_str.end()).value();

            pos = it;
            return ast;
        }

        return std::nullopt;
    }

    constexpr std::optional< int_type > try_parse_integral_keyword(std::string const& input)
    {
        auto it = input.begin();
        return try_parse_integral_keyword(it, input.end());
    }

    template < typename It >
    constexpr std::optional< float_type > try_parse_float_keyword(It& pos, It end)
    {
        auto it = pos;

        if (it == end || *it != 'F')
        {
            return std::nullopt;
        }
        ++it;

        auto bits_start = it;
        while (it != end && is_digit(*it))
        {
            ++it;
        }
        if (bits_start == it)
        {
            return std::nullopt;
        }

        auto bits = static_cast< std::size_t >(str_to_i(bits_start, it).value());
        std::size_t exponent_bits = 0;

        if (it != end && *it == 'E')
        {
            ++it;
            auto exponent_start = it;
            while (it != end && is_digit(*it))
            {
                ++it;
            }
            if (exponent_start == it)
            {
                throw std::logic_error("Expected exponent bit count after floating point type exponent marker");
            }
            exponent_bits = static_cast< std::size_t >(str_to_i(exponent_start, it).value());
        }
        else if (bits == 32)
        {
            exponent_bits = 8;
        }
        else if (bits == 64)
        {
            exponent_bits = 11;
        }
        else
        {
            throw std::logic_error("Floating point type shorthand is only available for F32 and F64; use F<N>E<M> for other sizes");
        }

        if (bits < 3 || exponent_bits == 0 || exponent_bits + 1 >= bits)
        {
            throw std::logic_error("Invalid floating point type bit layout");
        }

        pos = it;
        return float_type{.bits = bits, .exponent_bits = exponent_bits};
    }

    constexpr std::optional< float_type > try_parse_float_keyword(std::string const& input)
    {
        auto it = input.begin();
        return try_parse_float_keyword(it, input.end());
    }

    static_assert(try_parse_integral_keyword("I32")->bits == 32);
    static_assert(try_parse_integral_keyword("U32")->bits == 32);
    static_assert(try_parse_integral_keyword("I32")->has_sign);
    static_assert(!try_parse_integral_keyword("U32")->has_sign);
    static_assert(!try_parse_integral_keyword("BOOL").has_value());
    static_assert(!try_parse_integral_keyword("i32").has_value());
    static_assert(try_parse_float_keyword("F32")->bits == 32);
    static_assert(try_parse_float_keyword("F32")->exponent_bits == 8);
    static_assert(try_parse_float_keyword("F64")->bits == 64);
    static_assert(try_parse_float_keyword("F64")->exponent_bits == 11);
    static_assert(try_parse_float_keyword("F32E8")->bits == 32);
    static_assert(try_parse_float_keyword("F32E8")->exponent_bits == 8);

    // TODO: Fix this
    //static_assert(!try_parse_integral_keyword("I32U32").has_value());

} // namespace quxlang

#endif // TRY_PARSE_INTEGRAL_KEYWORD_HPP
