// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_FIXED_BYTEMATH_HEADER_GUARD
#define QUXLANG_FIXED_BYTEMATH_HEADER_GUARD
#include <vector>

#include <quxlang/bytemath.hpp>

namespace quxlang::bytemath
{

    struct fixed_int_options
    {
        bool has_sign = false;
        bool overflow_undefined = false;
        std::size_t bits = 0;
    };

    struct int_result
    {
        std::vector< std::byte > data_bytes;
        bool result_is_undefined;
    };

    struct bool_result
    {
        bool result;
        bool result_is_undefined;
    };

    inline auto get_bit(std::vector< std::byte > const& v, std::size_t bit) -> bool
    {
        std::size_t byte_index = bit / 8;
        std::size_t bit_index = bit % 8;
        if (byte_index >= v.size())
        {
            throw std::runtime_error("int_add_le: index out of range");
        }
        return (std::to_integer< std::uint8_t >(v[byte_index]) >> bit_index) & 1;
    }
    inline auto set_bit(std::vector< std::byte >& v, std::size_t bit, bool value) -> void
    {
        std::size_t byte_index = bit / 8;
        std::size_t bit_index = bit % 8;
        if (byte_index >= v.size())
        {
            throw std::runtime_error("int_add_le: index out of range");
        }
        if (value)
        {
            v[byte_index] |= std::byte(1 << bit_index);
        }
        else
        {
            v[byte_index] &= std::byte(~(1 << bit_index));
        }
    }

    inline sle_int_unlimited le_int_fixed_to_unlimited(fixed_int_options opt, std::vector< std::byte > const& input)
    {
        assert(input.size() == (opt.bits + 7) / 8);

        sle_int_unlimited result = 0;
        if (opt.has_sign)
        {
            result.is_negative = get_bit(input, opt.bits - 1);
        }
        else
        {
            result.is_negative = false;
        }

        if (result.is_negative)
        {
            std::vector< std::byte > abs_val;
            // In a N-bit two's complement, the largest unsigned absolute value is
            // N-bits.
            // To get the absolute value, we reverse the bits and then add 1.
            // This means for an N-bit value, the largest absolute value is
            // 1 + 000...000 (1 sign bit plus N-1 0 bits)
            // Producing a value 2^n -1 + 1 = 2^n

            // I.e. the range of the two's complement N-bit signed integer is
            // [-2^(N-1), 2^(N-1)-1]
            // The largest absolute value is 2^(N-1), which is less then 2^N-1
            // therefore can bit in N bits unsigned.

            // Since a byte can store up to 8 bits, we need ceil(N/8) bytes to store N bits.
            // however, in C++, `q/d` is floor(q/d) mathematically.
            // Ceil(q/d) = floor((q + d - 1) / d)
            // Given d = 8, we have:
            // Ceil(N/8) = floor((N + 7) / 8)
            // Thus in C++ we can compute this as (N + 7) / 8

            abs_val.resize((opt.bits + 7) / 8);

            // Conversion from negative Two's Complement to absolute value
            // https://en.wikipedia.org/wiki/Two%27s_complement

            // Step one, add the inverse of each bit to the relevant bit-position
            for (std::size_t i = 0; i < opt.bits - 1; i++)
            {
                auto bit = get_bit(input, i);
                set_bit(abs_val, i, !bit);
            }
            std::vector< std::byte > one{1};
            detail::le_trim_raw(abs_val);
            abs_val = detail::unlimited_int_unsigned_add_le_raw(std::move(abs_val), std::move(one));
            detail::le_trim_raw(abs_val);
            result.data = std::move(abs_val);
        }
        else
        {
            auto input_copy = input;
            detail::le_trim_raw(input_copy);
            result.data = std::move(input_copy);
        }
        return result;
    }

    inline int_result unlimited_to_fixed(fixed_int_options opt, sle_int_unlimited input)
    {
        std::vector< std::byte > output;
        output.resize((opt.bits + 7) / 8);

        if (input.is_negative)
        {
            if (opt.has_sign == false && opt.overflow_undefined)
            {
                // cannot represent negative number in unsigned type
                return {{}, true};
            }

            // For negative numbers, we want to compute the 2's complement representation.
            // The value is -|input|.
            // In N-bit 2's complement, -x is represented as 2^N - x.
            // However, we are truncating to opt.bits.
            // So we effectively want (-|input|) mod 2^opt.bits.
            // Which is equivalent to (2^opt.bits - (|input| mod 2^opt.bits)) mod 2^opt.bits.

            // A simpler way to think about it for implementation:
            // 1. Get absolute value (magnitude).
            // 2. Subtract 1 from magnitude.
            // 3. Invert bits of (magnitude - 1) to get the 2's complement form.

            std::vector< std::byte > one{std::byte{1}};
            detail::le_trim_raw(input.data);

            // We need to work with at least opt.bits precision to check for overflow if requested,
            // or just enough to capture the value.

            auto magnitude_minus_one = detail::unlimited_int_unsigned_sub_le_raw(input.data, std::move(one));

            // Check overflow if required
            if (opt.overflow_undefined)
            {
                // For signed N-bit integer, range is [-2^(N-1), 2^(N-1)-1].
                // Magnitude must be <= 2^(N-1).
                // So magnitude_minus_one must be < 2^(N-1).
                // i.e. bit (N-1) and above must be 0.

                // For unsigned N-bit integer, negative values are not representable (handled above).

                for (std::size_t i = opt.bits - 1; i < magnitude_minus_one.size() * 8; ++i)
                {
                    if (get_bit(magnitude_minus_one, i))
                    {
                        return {{}, true};
                    }
                }
            }

            // Generate output bits
            // The bits of the result are the inverse of the bits of (magnitude - 1)
            for (std::size_t i = 0; i < opt.bits; ++i)
            {
                bool bit_val = false;
                if (i < magnitude_minus_one.size() * 8)
                {
                    bit_val = get_bit(magnitude_minus_one, i);
                }
                // Invert bit
                set_bit(output, i, !bit_val);
            }

            return {std::move(output), false};
        }
        else // positive/zero result
        {
            if (opt.overflow_undefined)
            {
                std::size_t bit_limit;
                if (opt.has_sign)
                {
                    bit_limit = opt.bits - 1;
                }
                else
                {
                    bit_limit = opt.bits;
                }
                for (std::size_t i = bit_limit; i < input.data.size() * 8; ++i)
                {
                    if (get_bit(input.data, i))
                    {
                        return {{}, true};
                    }
                }
            }

            // Copy bits
            for (std::size_t i = 0; i < opt.bits; ++i)
            {
                bool bit_val = false;
                if (i < input.data.size() * 8)
                {
                    bit_val = get_bit(input.data, i);
                }
                set_bit(output, i, bit_val);
            }

            return {std::move(output), false};
        }
    }

    inline int_result fixed_int_add_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        assert(a.size() == (opt.bits + 7) / 8);
        assert(b.size() == (opt.bits + 7) / 8);

        sle_int_unlimited a_signed_unlimited = le_int_fixed_to_unlimited(opt, a);
        sle_int_unlimited b_signed_unlimited = le_int_fixed_to_unlimited(opt, b);

        sle_int_unlimited signed_result = unlimited_int_signed_add_le(std::move(a_signed_unlimited), std::move(b_signed_unlimited));

        return unlimited_to_fixed(opt, std::move(signed_result));
    }

    inline int_result int_sub_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return unlimited_to_fixed(opt, unlimited_int_signed_sub_le(le_int_fixed_to_unlimited(opt, a), le_int_fixed_to_unlimited(opt, b)));
    }
    // inline int_result int_mult_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    // {
    //     return unlimited_to_fixed(opt, le_signed_mult(le_int_fixed_to_unlimited(opt, a), le_int_fixed_to_unlimited(opt, b)));
    //     //}

    inline int_result int_div_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return unlimited_to_fixed(opt, le_signed_div(le_int_fixed_to_unlimited(opt, a), le_int_fixed_to_unlimited(opt, b)));
    }

    // Convert a fixed-width integer value from one set of options to another.
    // The input is interpreted using from_opt and the result is encoded using to_opt.
    // If the value cannot be represented under to_opt and overflow is undefined for to_opt,
    // result_is_undefined will be true.
    inline int_result fixed_int_convert(fixed_int_options from_opt, fixed_int_options to_opt, std::vector< std::byte > const& input)
    {
        assert(input.size() == (from_opt.bits + 7) / 8);
        sle_int_unlimited unlimited = le_int_fixed_to_unlimited(from_opt, input);
        return unlimited_to_fixed(to_opt, std::move(unlimited));
    }

    template < typename I >
    std::pair< I, bool > unlimited_to_int(fixed_int_options opts, sle_int_unlimited input)
    {
        auto res = unlimited_to_fixed(opts, input);
        ;
        if (res.result_is_undefined)
        {
            return {{}, false};
        }

        I result = 0;
        for (std::size_t i = 0; i < res.data_bytes.size(); ++i)
        {
            auto byte = std::to_integer< I >(std::byte(res.data_bytes[i]));
            result |= (byte << (i * 8));
        }

        return {result, true};
    }

} // namespace quxlang::bytemath

#endif // QUXLANG_FIXED_BYTEMATH_H
