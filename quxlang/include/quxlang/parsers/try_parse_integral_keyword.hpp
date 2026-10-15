// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_INTEGRAL_KEYWORD_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_INTEGRAL_KEYWORD_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <optional>
#include <quxlang/manipulators/numeric_literal_utils.hpp>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
{
    template < typename It >
    inline std::optional< int_type > try_parse_integral_keyword(It& pos, It end)
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

            auto [bits, representable] = bytemath::le_to_u< std::uint64_t >(literal_to_sle(dig_str).data);
            if (!representable)
            {
                throw syntax_compilation_error("Integer type bit count is too large");
            }
            ast.bits = bits;

            pos = it;
            return ast;
        }

        return std::nullopt;
    }

    inline std::optional< int_type > try_parse_integral_keyword(std::string const& input)
    {
        auto it = input.begin();
        return try_parse_integral_keyword(it, input.end());
    }

    template < typename It >
    inline std::optional< float_type > try_parse_float_keyword(It& pos, It end)
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

        auto [bits, representable] = bytemath::le_to_u< std::uint64_t >(literal_to_sle(std::string(bits_start, it)).data);
        if (!representable)
        {
            throw syntax_compilation_error("Floating point type bit count is too large");
        }
        std::uint64_t exponent_bits = 0;

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
                throw syntax_compilation_error("Expected exponent bit count after floating point type exponent marker");
            }
            auto [parsed_exponent_bits, exponent_representable] = bytemath::le_to_u< std::uint64_t >(literal_to_sle(std::string(exponent_start, it)).data);
            if (!exponent_representable)
            {
                throw syntax_compilation_error("Floating point exponent bit count is too large");
            }
            exponent_bits = parsed_exponent_bits;
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
            throw syntax_compilation_error("Floating point type shorthand is only available for F32 and F64; use F<N>E<M> for other sizes");
        }

        if (bits < 3 || exponent_bits == 0 || exponent_bits >= bits - 1)
        {
            throw syntax_compilation_error("Invalid floating point type bit layout");
        }

        pos = it;
        return float_type{.bits = bits, .exponent_bits = exponent_bits};
    }

    inline std::optional< float_type > try_parse_float_keyword(std::string const& input)
    {
        auto it = input.begin();
        return try_parse_float_keyword(it, input.end());
    }


} // namespace quxlang

#endif // TRY_PARSE_INTEGRAL_KEYWORD_HPP
