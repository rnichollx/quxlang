//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef QUXLANG_PARSERS_TRY_PARSE_INTEGRAL_KEYWORD_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_INTEGRAL_KEYWORD_HEADER_GUARD

#include <optional>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/parsers/ctype.hpp>

namespace quxlang::parsers
{
    template < typename It >
    constexpr std::optional< primitive_type_integer_reference > try_parse_integral_keyword(It& pos, It end)
    {
        primitive_type_integer_reference ast{};
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

            ast.bits = str_to_i(dig_str.begin(), dig_str.end()).value();

            pos = it;
            return ast;
        }

        return std::nullopt;
    }

    constexpr std::optional< primitive_type_integer_reference > try_parse_integral_keyword(std::string const& input)
    {
        auto it = input.begin();
        return try_parse_integral_keyword(it, input.end());
    }

    static_assert(try_parse_integral_keyword("I32")->bits == 32);
    static_assert(try_parse_integral_keyword("U32")->bits == 32);
    static_assert(try_parse_integral_keyword("I32")->has_sign);
    static_assert(!try_parse_integral_keyword("U32")->has_sign);
    static_assert(!try_parse_integral_keyword("BOOL").has_value());
    static_assert(!try_parse_integral_keyword("i32").has_value());

    // TODO: Fix this
    //static_assert(!try_parse_integral_keyword("I32U32").has_value());

} // namespace quxlang

#endif // TRY_PARSE_INTEGRAL_KEYWORD_HPP
