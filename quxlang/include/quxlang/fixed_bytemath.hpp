// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_FIXED_BYTEMATH_H
#define QUXLANG_FIXED_BYTEMATH_H
#include <vector>

#include <quxlang/bytemath.hpp>

namespace quxlang::bytemath
{

    struct fixed_int_options
    {
        bool has_sign;
        bool overflow_undefined;
        std::size_t bits;
    };

    struct int_result
    {
        std::vector< std::byte > result;
        bool result_is_undefined;
    };

    struct bool_result
    {
        bool result;
        bool result_is_undefined;
    };

    auto get_bit(std::vector< std::byte > const& v, std::size_t bit) -> bool
    {
        std::size_t byte_index = bit / 8;
        std::size_t bit_index = bit % 8;
        if (byte_index >= v.size())
        {
            throw std::runtime_error("int_add_le: index out of range");
        }
        return (std::to_integer< std::uint8_t >(v[byte_index]) >> bit_index) & 1;
    }
    auto set_bit(std::vector< std::byte >& v, std::size_t bit, bool value) -> void
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
            le_trim(abs_val);
            abs_val = unlimited_int_unsigned_add_le(std::move(abs_val), std::move(one));
            le_trim(abs_val);
            result.data = std::move(abs_val);
        }
        else
        {
            auto input_copy = input;
            le_trim(input_copy);
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
            set_bit(output, opt.bits - 1, true);

            // Convert to two's complement

            std::vector< std::byte > one{1};
            le_trim(input.data);

            auto unsigned_magnituide = unlimited_int_unsigned_sub_le(input.data, std::move(one));
            if (unsigned_magnituide.size() < (opt.bits + 7) / 8)
            {
                unsigned_magnituide.resize((opt.bits + 7) / 8);
            }
            for (std::size_t i = 0; i < opt.bits - 1; i++)
            {
                auto bit = get_bit(unsigned_magnituide, i);
                set_bit(output, i, !bit);
                // Clear bits from unsigned magnituide as we go
                set_bit(unsigned_magnituide, i, false);
            }
            // Because we clear bits from the unsigned magnitude as we put them back into the two's complement version,
            // any remaining bits indicates overflow!
            if (opt.overflow_undefined)
            {
                for (std::byte b : unsigned_magnituide)
                {
                    if (b != std::byte(0))
                    {
                        return {{}, true};
                    }
                }
            }

            return {std::move(output), false};
        }
        else // positive/zero result
        {
            // encode non-negative in two's complement, ensure it fits in width-1
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
            for (std::size_t i = 0; i < std::min< std::size_t >(opt.bits - 1, input.data.size() * 8); ++i)
            {
                set_bit(output, i, get_bit(input.data, i));
            }
            set_bit(output, opt.bits - 1, false);
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


} // namespace quxlang::bytemath

#endif // QUXLANG_FIXED_BYTEMATH_H
