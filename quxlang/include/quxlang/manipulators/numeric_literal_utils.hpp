// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_NUMERIC_LITERAL_UTILS_HEADER_GUARD
#define QUXLANG_MANIPULATORS_NUMERIC_LITERAL_UTILS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/bytemath.hpp>
#include <quxlang/fixed_bytemath.hpp>

#include <cctype>
#include <stdexcept>
#include <string>

namespace quxlang
{
    inline bool is_integer_literal(std::string const& text)
    {
        std::size_t i = 0;
        if (i < text.size() && (text[i] == '+' || text[i] == '-'))
            ++i;
        if (i == text.size())
            return false;
        for (; i < text.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(text[i])))
                return false;
        }
        return true;
    }

    inline bool is_float_literal(std::string const& text)
    {
        return !is_integer_literal(text) && !text.empty();
    }

    // Parse a signed decimal literal string into a bytemath unlimited signed integer.
    inline bytemath::sle_int_unlimited literal_to_sle(std::string const& text)
    {
        if (!is_integer_literal(text))
            throw std::invalid_argument("not an integer literal: " + text);

        bool negative = false;
        std::string digits = text;
        if (!digits.empty() && (digits[0] == '-' || digits[0] == '+'))
        {
            negative = digits[0] == '-';
            digits = digits.substr(1);
        }

        auto data = bytemath::detail::string_to_le_raw(digits);
        bytemath::sle_int_unlimited value{std::move(data), negative};
        return bytemath::normalize_signed(std::move(value));
    }

    // Render a bytemath unlimited signed integer back to a decimal literal string.
    inline std::string sle_to_literal(bytemath::sle_int_unlimited value)
    {
        value = bytemath::normalize_signed(std::move(value));
        std::string result = bytemath::detail::le_to_string_raw(value.data);
        if (value.is_negative && result != "0")
            result = "-" + result;
        return result;
    }

    inline bool literal_fits_int(std::string const& digits, int_type const& target)
    {
        if (!is_integer_literal(digits))
            return false;
        if (target.bits == 0)
            return false;

        bytemath::sle_int_unlimited value;
        try
        {
            value = literal_to_sle(digits);
        }
        catch (...)
        {
            return false;
        }

        bytemath::fixed_int_options opt;
        opt.has_sign = target.has_sign;
        opt.overflow_undefined = true;
        opt.bits = target.bits;

        auto res = bytemath::unlimited_to_fixed(opt, std::move(value));
        return !res.result_is_undefined;
    }

    inline bool literal_fits_float(std::string const& digits, float_type const& target)
    {
        if (digits.empty())
            return false;

        // Accept any numeric literal as fitting a float via APPROXIMATE. The
        // distinction between exact and approximate float conversion is handled
        // at the adaptation level (APPROXIMATE vs implicit). Range/precision
        // checking would require IEEE float analysis.
        return is_integer_literal(digits) || is_float_literal(digits);
    }

    inline int literal_compare(std::string const& lhs, std::string const& rhs)
    {
        auto a = literal_to_sle(lhs);
        auto b = literal_to_sle(rhs);
        if (a < b)
            return -1;
        if (b < a)
            return 1;
        return 0;
    }

    inline std::string literal_add(std::string const& lhs, std::string const& rhs)
    {
        return sle_to_literal(bytemath::signed_add(literal_to_sle(lhs), literal_to_sle(rhs)));
    }

    inline std::string literal_subtract(std::string const& lhs, std::string const& rhs)
    {
        return sle_to_literal(bytemath::signed_sub(literal_to_sle(lhs), literal_to_sle(rhs)));
    }

    inline std::string literal_negate(std::string const& operand)
    {
        return sle_to_literal(bytemath::signed_negate(literal_to_sle(operand)));
    }

    inline std::string literal_multiply(std::string const& lhs, std::string const& rhs)
    {
        return sle_to_literal(bytemath::le_signed_mult(literal_to_sle(lhs), literal_to_sle(rhs)));
    }

    inline std::string literal_divide(std::string const& lhs, std::string const& rhs)
    {
        auto a = literal_to_sle(lhs);
        auto b = literal_to_sle(rhs);
        if (bytemath::raw_is_zero(b.data))
            throw std::invalid_argument("Division by zero in numeric literal");
        return sle_to_literal(bytemath::le_signed_div(std::move(a), std::move(b)));
    }

    inline std::string literal_modulus(std::string const& lhs, std::string const& rhs)
    {
        auto a = literal_to_sle(lhs);
        auto b = literal_to_sle(rhs);
        if (bytemath::raw_is_zero(b.data))
            throw std::invalid_argument("Modulus by zero in numeric literal");
        // remainder = a - (a / b) * b  (C-style truncated division)
        auto quotient = bytemath::le_signed_div(a, b);
        auto product = bytemath::le_signed_mult(b, quotient);
        auto remainder = bytemath::signed_sub(std::move(a), std::move(product));
        return sle_to_literal(std::move(remainder));
    }
} // namespace quxlang

#endif // QUXLANG_MANIPULATORS_NUMERIC_LITERAL_UTILS_HEADER_GUARD
