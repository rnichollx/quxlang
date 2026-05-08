// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_FIXED_BYTEMATH_HEADER_GUARD
#define QUXLANG_FIXED_BYTEMATH_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <cstdint>
#include <limits>
#include <stdexcept>
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

    struct fixed_float_options
    {
        std::size_t bits = 0;
        std::size_t exponent_bits = 0;
    };

    struct float_result
    {
        std::vector< std::byte > data_bytes;
        bool result_is_undefined = false;
        bool result_is_exact = true;
    };

    inline auto get_bit(std::vector< std::byte > const& v, std::size_t bit) -> bool
    {
        std::size_t byte_index = bit / 8;
        std::size_t bit_index = bit % 8;
        if (byte_index >= v.size())
        {
            throw quxlang::compiler_bug("int_add_le: index out of range");
        }
        return (std::to_integer< std::uint8_t >(v[byte_index]) >> bit_index) & 1;
    }
    inline auto set_bit(std::vector< std::byte >& v, std::size_t bit, bool value) -> void
    {
        std::size_t byte_index = bit / 8;
        std::size_t bit_index = bit % 8;
        if (byte_index >= v.size())
        {
            throw quxlang::compiler_bug("int_add_le: index out of range");
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
            for (std::size_t i = 0; i < opt.bits; i++)
            {
                auto bit = get_bit(input, i);
                set_bit(abs_val, i, !bit);
            }
            std::vector< std::byte > one{std::byte{1}};
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

    inline bool le_is_zero(std::vector< std::byte > const& value)
    {
        return std::all_of(value.begin(), value.end(),
                           [](std::byte byte)
                           {
                               return byte == std::byte{0};
                           });
    }

    inline int_result fixed_int_sub_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return unlimited_to_fixed(opt, unlimited_int_signed_sub_le(le_int_fixed_to_unlimited(opt, a), le_int_fixed_to_unlimited(opt, b)));
    }

    inline int_result fixed_int_mul_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return unlimited_to_fixed(opt, le_signed_mult(le_int_fixed_to_unlimited(opt, a), le_int_fixed_to_unlimited(opt, b)));
    }

    inline int_result fixed_int_div_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        if (le_is_zero(b))
        {
            return {{}, true};
        }

        return unlimited_to_fixed(opt, le_signed_div(le_int_fixed_to_unlimited(opt, a), le_int_fixed_to_unlimited(opt, b)));
    }

    inline int_result fixed_int_mod_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        if (le_is_zero(b))
        {
            return {{}, true};
        }

        auto dividend = le_int_fixed_to_unlimited(opt, a);
        auto divisor = le_int_fixed_to_unlimited(opt, b);

        if (divisor.is_negative)
        {
            return {{}, true};
        }

        auto quotient = le_signed_div(dividend, divisor);
        auto product = le_signed_mult(divisor, quotient);
        auto remainder = unlimited_int_signed_sub_le(std::move(dividend), std::move(product));
        return unlimited_to_fixed(opt, std::move(remainder));
    }

    inline int_result fixed_int_shift_up_le(fixed_int_options opt, std::vector< std::byte > value, std::uint64_t amount)
    {
        std::size_t result_size = (opt.bits + 7) / 8;

        if (amount >= opt.bits)
        {
            if (opt.overflow_undefined)
            {
                return {{}, true};
            }
            return {std::vector< std::byte >(result_size, std::byte{0}), false};
        }

        auto shifted = detail::le_shift_up_raw(std::move(value), static_cast< std::size_t >(amount));
        shifted = detail::le_truncate_raw(std::move(shifted), opt.bits);
        shifted.resize(result_size, std::byte{0});
        return {std::move(shifted), false};
    }

    inline int_result fixed_int_shift_down_le(fixed_int_options opt, std::vector< std::byte > value, std::uint64_t amount)
    {
        std::size_t result_size = (opt.bits + 7) / 8;

        if (amount >= opt.bits)
        {
            if (opt.overflow_undefined)
            {
                return {{}, true};
            }
            return {std::vector< std::byte >(result_size, std::byte{0}), false};
        }

        auto shifted = detail::le_shift_down_raw(std::move(value), static_cast< std::size_t >(amount));
        shifted = detail::le_truncate_raw(std::move(shifted), opt.bits);
        shifted.resize(result_size, std::byte{0});
        return {std::move(shifted), false};
    }

    inline int_result int_sub_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return fixed_int_sub_le(opt, std::move(a), std::move(b));
    }

    inline int_result int_div_le(fixed_int_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return fixed_int_div_le(opt, std::move(a), std::move(b));
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

    enum class fixed_float_kind
    {
        zero,
        finite,
        infinity,
        nan,
    };

    struct unpacked_fixed_float
    {
        bool negative = false;
        fixed_float_kind kind = fixed_float_kind::zero;
        std::vector< std::byte > significand = {std::byte{0}};
        sle_int_unlimited scale = 0;
    };

    inline void raw_trim(std::vector< std::byte >& value)
    {
        detail::le_trim_raw(value);
    }

    inline bool raw_is_zero(std::vector< std::byte > const& value)
    {
        return le_is_zero(value);
    }

    inline std::vector< std::byte > raw_one()
    {
        return {std::byte{1}};
    }

    inline std::size_t raw_bit_width(std::vector< std::byte > value)
    {
        raw_trim(value);
        if (raw_is_zero(value))
        {
            return 0;
        }

        std::size_t result = (value.size() - 1) * 8;
        std::uint8_t high_byte = std::to_integer< std::uint8_t >(value.back());
        while (high_byte != 0)
        {
            ++result;
            high_byte >>= 1;
        }
        return result;
    }

    inline bool raw_get_bit(std::vector< std::byte > const& value, std::size_t bit)
    {
        std::size_t byte_index = bit / 8;
        if (byte_index >= value.size())
        {
            return false;
        }
        return (std::to_integer< std::uint8_t >(value[byte_index]) >> (bit % 8)) & 1;
    }

    inline std::vector< std::byte > raw_power_of_two(std::size_t bit)
    {
        std::vector< std::byte > result(bit / 8 + 1, std::byte{0});
        set_bit(result, bit, true);
        return result;
    }

    inline std::vector< std::byte > raw_power_of_two_minus_one(std::size_t bit_count)
    {
        if (bit_count == 0)
        {
            return {std::byte{0}};
        }

        auto result = raw_power_of_two(bit_count);
        result = detail::unlimited_int_unsigned_sub_le_raw(std::move(result), raw_one());
        raw_trim(result);
        return result;
    }

    inline sle_int_unlimited normalize_signed(sle_int_unlimited value)
    {
        raw_trim(value.data);
        if (raw_is_zero(value.data))
        {
            value.is_negative = false;
        }
        return value;
    }

    inline sle_int_unlimited signed_from_size(std::size_t value)
    {
        return normalize_signed(sle_int_unlimited(value));
    }

    inline sle_int_unlimited signed_negate(sle_int_unlimited value)
    {
        value = normalize_signed(std::move(value));
        if (!raw_is_zero(value.data))
        {
            value.is_negative = !value.is_negative;
        }
        return value;
    }

    inline sle_int_unlimited signed_add(sle_int_unlimited a, sle_int_unlimited b);

    inline sle_int_unlimited signed_sub(sle_int_unlimited a, sle_int_unlimited b)
    {
        b = signed_negate(std::move(b));
        return signed_add(std::move(a), std::move(b));
    }

    inline sle_int_unlimited signed_add(sle_int_unlimited a, sle_int_unlimited b)
    {
        a = normalize_signed(std::move(a));
        b = normalize_signed(std::move(b));

        if (a.is_negative == b.is_negative)
        {
            return normalize_signed(sle_int_unlimited(detail::unlimited_int_unsigned_add_le_raw(std::move(a.data), std::move(b.data)), a.is_negative));
        }

        if (detail::le_comp_less_raw(a.data, b.data))
        {
            return normalize_signed(sle_int_unlimited(detail::unlimited_int_unsigned_sub_le_raw(std::move(b.data), std::move(a.data)), b.is_negative));
        }

        return normalize_signed(sle_int_unlimited(detail::unlimited_int_unsigned_sub_le_raw(std::move(a.data), std::move(b.data)), a.is_negative));
    }

    inline sle_int_unlimited signed_sub_size(sle_int_unlimited value, std::size_t amount)
    {
        return signed_sub(std::move(value), signed_from_size(amount));
    }

    inline sle_int_unlimited signed_add_size(sle_int_unlimited value, std::size_t amount)
    {
        return signed_add(std::move(value), signed_from_size(amount));
    }

    inline bool signed_less(sle_int_unlimited const& a, sle_int_unlimited const& b)
    {
        return a < b;
    }

    inline bool signed_equal(sle_int_unlimited const& a, sle_int_unlimited const& b)
    {
        return a == b;
    }

    inline std::size_t signed_nonnegative_to_size(sle_int_unlimited value)
    {
        value = normalize_signed(std::move(value));
        if (value.is_negative)
        {
            throw quxlang::compiler_bug("negative value cannot be converted to size");
        }
        auto [result, ok] = detail::le_to_u_raw< std::size_t >(value.data);
        if (!ok)
        {
            throw quxlang::compiler_bug("fixed float shift is too large for host allocation");
        }
        return result;
    }

    inline bool signed_greater_than_size(sle_int_unlimited value, std::size_t amount)
    {
        return signed_less(signed_from_size(amount), normalize_signed(std::move(value)));
    }

    inline bool signed_nonnegative_fits_size(sle_int_unlimited value)
    {
        value = normalize_signed(std::move(value));
        if (value.is_negative)
        {
            return false;
        }

        auto [result, ok] = detail::le_to_u_raw< std::size_t >(value.data);
        (void)result;
        return ok;
    }

    inline sle_int_unlimited signed_abs_difference(sle_int_unlimited const& a, sle_int_unlimited const& b)
    {
        if (signed_less(a, b))
        {
            return signed_sub(b, a);
        }
        return signed_sub(a, b);
    }

    inline std::vector< std::byte > raw_from_u64(std::uint64_t value)
    {
        auto result = detail::u_to_le_raw(value);
        if (result.empty())
        {
            result.push_back(std::byte{0});
        }
        return result;
    }

    inline void validate_fixed_float_options(fixed_float_options opt)
    {
        if (opt.bits < 3 || opt.exponent_bits == 0 || opt.exponent_bits + 1 >= opt.bits)
        {
            throw quxlang::compiler_bug("invalid fixed float options");
        }
    }

    inline std::size_t fixed_float_significand_bits(fixed_float_options opt)
    {
        validate_fixed_float_options(opt);
        return opt.bits - opt.exponent_bits - 1;
    }

    inline std::vector< std::byte > fixed_float_exponent_bias_raw(fixed_float_options opt)
    {
        validate_fixed_float_options(opt);
        return raw_power_of_two_minus_one(opt.exponent_bits - 1);
    }

    inline std::vector< std::byte > fixed_float_max_exponent_raw(fixed_float_options opt)
    {
        validate_fixed_float_options(opt);
        return raw_power_of_two_minus_one(opt.exponent_bits);
    }

    inline sle_int_unlimited fixed_float_exponent_bias_signed(fixed_float_options opt)
    {
        return sle_int_unlimited(fixed_float_exponent_bias_raw(opt), false);
    }

    inline sle_int_unlimited fixed_float_min_normal_exponent(fixed_float_options opt)
    {
        return signed_sub(sle_int_unlimited(1), fixed_float_exponent_bias_signed(opt));
    }

    inline sle_int_unlimited fixed_float_max_normal_exponent(fixed_float_options opt)
    {
        return fixed_float_exponent_bias_signed(opt);
    }

    inline std::vector< std::byte > read_bit_field(std::vector< std::byte > const& input, std::size_t offset, std::size_t bits)
    {
        std::vector< std::byte > result((bits + 7) / 8, std::byte{0});
        if (result.empty())
        {
            result.push_back(std::byte{0});
        }

        for (std::size_t i = 0; i < bits; ++i)
        {
            if (raw_get_bit(input, offset + i))
            {
                set_bit(result, i, true);
            }
        }
        raw_trim(result);
        return result;
    }

    inline void write_bit_field(std::vector< std::byte >& output, std::size_t offset, std::size_t bits, std::vector< std::byte > const& value)
    {
        for (std::size_t i = 0; i < bits; ++i)
        {
            set_bit(output, offset + i, raw_get_bit(value, i));
        }
    }

    inline std::vector< std::byte > pack_fixed_float_bits(fixed_float_options opt, bool negative, std::vector< std::byte > exponent, std::vector< std::byte > fraction)
    {
        validate_fixed_float_options(opt);
        std::size_t const significand_bits = fixed_float_significand_bits(opt);
        std::vector< std::byte > output((opt.bits + 7) / 8, std::byte{0});

        write_bit_field(output, 0, significand_bits, fraction);
        write_bit_field(output, significand_bits, opt.exponent_bits, exponent);
        set_bit(output, opt.bits - 1, negative);
        return output;
    }

    inline std::vector< std::byte > fixed_float_clear_padding_bits_le(fixed_float_options opt, std::vector< std::byte > value)
    {
        validate_fixed_float_options(opt);
        value.resize((opt.bits + 7) / 8, std::byte{0});

        std::size_t const final_byte_bits = opt.bits % 8;
        if (final_byte_bits != 0)
        {
            auto const mask = static_cast< std::uint8_t >((std::uint16_t{1} << final_byte_bits) - 1);
            value.back() &= std::byte{mask};
        }

        return value;
    }

    inline std::vector< std::byte > shift_left_by_signed_distance(std::vector< std::byte > value, sle_int_unlimited shift)
    {
        shift = normalize_signed(std::move(shift));
        if (shift.is_negative)
        {
            throw quxlang::compiler_bug("negative shift in fixed float alignment");
        }
        return detail::le_shift_up_raw(std::move(value), signed_nonnegative_to_size(std::move(shift)));
    }

    inline float_result fixed_float_zero(fixed_float_options opt, bool negative = false)
    {
        return {pack_fixed_float_bits(opt, negative, {std::byte{0}}, {std::byte{0}}), false};
    }

    inline float_result fixed_float_infinity(fixed_float_options opt, bool negative)
    {
        return {pack_fixed_float_bits(opt, negative, fixed_float_max_exponent_raw(opt), {std::byte{0}}), false};
    }

    inline float_result fixed_float_nan(fixed_float_options opt)
    {
        std::size_t const significand_bits = fixed_float_significand_bits(opt);
        auto payload = raw_power_of_two(significand_bits == 0 ? 0 : significand_bits - 1);
        return {pack_fixed_float_bits(opt, false, fixed_float_max_exponent_raw(opt), std::move(payload)), false};
    }

    inline unpacked_fixed_float unpack_fixed_float(fixed_float_options opt, std::vector< std::byte > const& input)
    {
        validate_fixed_float_options(opt);
        assert(input.size() == (opt.bits + 7) / 8);

        std::size_t const significand_bits = fixed_float_significand_bits(opt);
        auto exponent = read_bit_field(input, significand_bits, opt.exponent_bits);
        auto fraction = read_bit_field(input, 0, significand_bits);

        unpacked_fixed_float result;
        result.negative = raw_get_bit(input, opt.bits - 1);

        if (le_comp_eq(exponent, fixed_float_max_exponent_raw(opt)))
        {
            result.kind = raw_is_zero(fraction) ? fixed_float_kind::infinity : fixed_float_kind::nan;
            return result;
        }

        if (raw_is_zero(exponent))
        {
            if (raw_is_zero(fraction))
            {
                result.kind = fixed_float_kind::zero;
                return result;
            }

            result.kind = fixed_float_kind::finite;
            result.significand = std::move(fraction);
            result.scale = signed_sub_size(fixed_float_min_normal_exponent(opt), significand_bits);
            return result;
        }

        result.kind = fixed_float_kind::finite;
        result.significand = detail::unlimited_int_unsigned_add_le_raw(raw_power_of_two(significand_bits), std::move(fraction));
        result.scale = signed_sub_size(signed_sub(sle_int_unlimited(std::move(exponent), false), fixed_float_exponent_bias_signed(opt)), significand_bits);
        return result;
    }

    inline bool fixed_float_value_matches_rational(fixed_float_options opt, bool negative, std::vector< std::byte > numerator, std::vector< std::byte > denominator, sle_int_unlimited scale, std::vector< std::byte > const& value)
    {
        raw_trim(numerator);
        raw_trim(denominator);
        if (raw_is_zero(denominator))
        {
            return false;
        }

        auto unpacked = unpack_fixed_float(opt, value);
        if (raw_is_zero(numerator))
        {
            return unpacked.kind == fixed_float_kind::zero;
        }
        if (unpacked.kind != fixed_float_kind::finite || unpacked.negative != negative)
        {
            return false;
        }

        auto common_scale = signed_less(scale, unpacked.scale) ? scale : unpacked.scale;
        auto lhs = detail::le_unsigned_mult_raw(std::move(numerator), raw_one());
        lhs = shift_left_by_signed_distance(std::move(lhs), signed_sub(scale, common_scale));

        auto rhs = detail::le_unsigned_mult_raw(std::move(unpacked.significand), std::move(denominator));
        rhs = shift_left_by_signed_distance(std::move(rhs), signed_sub(unpacked.scale, std::move(common_scale)));
        return le_comp_eq(lhs, rhs);
    }

    inline sle_int_unlimited floor_log2_rational(std::vector< std::byte > const& numerator, std::vector< std::byte > const& denominator)
    {
        assert(!raw_is_zero(numerator));
        assert(!raw_is_zero(denominator));

        std::size_t numerator_bits = raw_bit_width(numerator);
        std::size_t denominator_bits = raw_bit_width(denominator);

        sle_int_unlimited candidate;
        bool less_than_power = false;

        if (numerator_bits >= denominator_bits)
        {
            std::size_t shift = numerator_bits - denominator_bits;
            candidate = signed_from_size(shift);
            auto threshold = detail::le_shift_up_raw(denominator, shift);
            less_than_power = detail::le_comp_less_raw(numerator, threshold);
        }
        else
        {
            std::size_t shift = denominator_bits - numerator_bits;
            candidate = signed_negate(signed_from_size(shift));
            auto shifted_numerator = detail::le_shift_up_raw(numerator, shift);
            less_than_power = detail::le_comp_less_raw(shifted_numerator, denominator);
        }

        if (less_than_power)
        {
            candidate = signed_sub(candidate, sle_int_unlimited(1));
        }
        return normalize_signed(std::move(candidate));
    }

    inline std::vector< std::byte > rounded_scaled_divide(std::vector< std::byte > numerator, std::vector< std::byte > denominator, sle_int_unlimited shift)
    {
        raw_trim(numerator);
        raw_trim(denominator);
        if (raw_is_zero(denominator))
        {
            throw quxlang::semantic_compilation_error("fixed float division by zero");
        }
        if (raw_is_zero(numerator))
        {
            return {std::byte{0}};
        }

        shift = normalize_signed(std::move(shift));
        if (signed_nonnegative_fits_size(shift.is_negative ? signed_negate(shift) : shift))
        {
            auto shift_size = signed_nonnegative_to_size(shift.is_negative ? signed_negate(shift) : shift);
            auto numerator_width = raw_bit_width(numerator);
            auto denominator_width = raw_bit_width(denominator);
            bool scaled_value_fits = shift_size < 64 && ((shift.is_negative && denominator_width <= 64 - shift_size) || (!shift.is_negative && numerator_width <= 64 - shift_size));
            if (scaled_value_fits)
            {
                auto [numerator_value, numerator_ok] = detail::le_to_u_raw< std::uint64_t >(numerator);
                auto [denominator_value, denominator_ok] = detail::le_to_u_raw< std::uint64_t >(denominator);
                if (numerator_ok && denominator_ok)
                {
                    if (shift.is_negative)
                    {
                        denominator_value <<= shift_size;
                    }
                    else
                    {
                        numerator_value <<= shift_size;
                    }

                    std::uint64_t quotient = numerator_value / denominator_value;
                    std::uint64_t remainder = numerator_value % denominator_value;
                    if (remainder == 0)
                    {
                        return raw_from_u64(quotient);
                    }

                    std::uint64_t half_denominator = denominator_value / 2;
                    bool round_up = remainder > half_denominator || (denominator_value % 2 == 0 && remainder == half_denominator && (quotient & 1) != 0);
                    if (round_up)
                    {
                        ++quotient;
                    }
                    return raw_from_u64(quotient);
                }
            }
        }

        if (shift.is_negative)
        {
            denominator = detail::le_shift_up_raw(std::move(denominator), signed_nonnegative_to_size(signed_negate(shift)));
        }
        else
        {
            numerator = detail::le_shift_up_raw(std::move(numerator), signed_nonnegative_to_size(shift));
        }

        auto [quotient, remainder] = detail::le_unsigned_divmod_raw(std::move(numerator), denominator);
        raw_trim(quotient);
        raw_trim(remainder);
        if (raw_is_zero(remainder))
        {
            return quotient;
        }

        auto twice_remainder = detail::le_shift_up_raw(remainder, 1);
        bool round_up = false;
        if (detail::le_comp_less_raw(denominator, twice_remainder))
        {
            round_up = true;
        }
        else if (le_comp_eq(denominator, twice_remainder) && raw_get_bit(quotient, 0))
        {
            round_up = true;
        }

        if (round_up)
        {
            quotient = detail::unlimited_int_unsigned_add_le_raw(std::move(quotient), raw_one());
        }
        raw_trim(quotient);
        return quotient;
    }

    inline float_result fixed_float_from_rational(fixed_float_options opt, bool negative, std::vector< std::byte > numerator, std::vector< std::byte > denominator, sle_int_unlimited scale, bool track_exactness = false)
    {
        validate_fixed_float_options(opt);
        raw_trim(numerator);
        raw_trim(denominator);

        std::vector< std::byte > exact_numerator;
        std::vector< std::byte > exact_denominator;
        sle_int_unlimited exact_scale;
        if (track_exactness)
        {
            exact_numerator = numerator;
            exact_denominator = denominator;
            exact_scale = scale;
        }
        auto finish_result = [&](float_result result) -> float_result
        {
            if (track_exactness)
            {
                result.result_is_exact = fixed_float_value_matches_rational(opt, negative, exact_numerator, exact_denominator, exact_scale, result.data_bytes);
            }
            return result;
        };

        if (raw_is_zero(numerator))
        {
            return finish_result(fixed_float_zero(opt, negative));
        }
        if (raw_is_zero(denominator))
        {
            return finish_result(fixed_float_infinity(opt, negative));
        }

        std::size_t const fraction_bits = fixed_float_significand_bits(opt);
        std::size_t const precision_bits = fraction_bits + 1;
        auto high_exponent = signed_add(floor_log2_rational(numerator, denominator), scale);
        auto min_normal = fixed_float_min_normal_exponent(opt);
        auto max_normal = fixed_float_max_normal_exponent(opt);

        if (signed_less(max_normal, high_exponent))
        {
            return finish_result(fixed_float_infinity(opt, negative));
        }

        if (signed_less(high_exponent, min_normal))
        {
            auto shift = signed_sub(signed_add_size(scale, fraction_bits), min_normal);
            if (shift.is_negative && signed_greater_than_size(signed_negate(shift), raw_bit_width(numerator) + 1))
            {
                return finish_result(fixed_float_zero(opt, negative));
            }

            auto fraction = rounded_scaled_divide(std::move(numerator), std::move(denominator), std::move(shift));
            std::size_t fraction_width = raw_bit_width(fraction);
            if (fraction_width == 0)
            {
                return finish_result(fixed_float_zero(opt, negative));
            }
            if (fraction_width > fraction_bits)
            {
                return finish_result({pack_fixed_float_bits(opt, negative, {std::byte{1}}, {std::byte{0}}), false});
            }
            return finish_result({pack_fixed_float_bits(opt, negative, {std::byte{0}}, std::move(fraction)), false});
        }

        auto shift = signed_sub(signed_add_size(scale, fraction_bits), high_exponent);
        auto significand = rounded_scaled_divide(std::move(numerator), std::move(denominator), std::move(shift));
        if (raw_bit_width(significand) > precision_bits)
        {
            significand = detail::le_shift_down_raw(std::move(significand), 1);
            high_exponent = signed_add(high_exponent, sle_int_unlimited(1));
            if (signed_less(max_normal, high_exponent))
            {
                return finish_result(fixed_float_infinity(opt, negative));
            }
        }

        auto exponent = signed_add(high_exponent, fixed_float_exponent_bias_signed(opt));
        if (exponent.is_negative || raw_is_zero(exponent.data))
        {
            throw quxlang::compiler_bug("normal fixed float packed with non-normal exponent");
        }

        auto fraction = detail::unlimited_int_unsigned_sub_le_raw(std::move(significand), raw_power_of_two(fraction_bits));
        return finish_result({pack_fixed_float_bits(opt, negative, std::move(exponent.data), std::move(fraction)), false});
    }

    inline float_result fixed_float_from_decimal_string(fixed_float_options opt, std::string const& value, bool track_exactness = false)
    {
        validate_fixed_float_options(opt);
        bool negative = false;
        std::size_t pos = 0;
        if (pos < value.size() && (value[pos] == '+' || value[pos] == '-'))
        {
            negative = value[pos] == '-';
            ++pos;
        }

        std::string digits;
        std::size_t fractional_digits = 0;
        bool after_decimal_point = false;
        for (; pos < value.size(); ++pos)
        {
            char c = value[pos];
            if (c == '.')
            {
                if (after_decimal_point)
                {
                    throw std::invalid_argument("invalid fixed float literal");
                }
                after_decimal_point = true;
                continue;
            }
            if (c < '0' || c > '9')
            {
                throw std::invalid_argument("invalid fixed float literal");
            }
            digits.push_back(c);
            if (after_decimal_point)
            {
                ++fractional_digits;
            }
        }

        if (digits.empty())
        {
            throw std::invalid_argument("invalid fixed float literal");
        }

        auto numerator = detail::string_to_le_raw(digits);
        auto denominator = raw_one();
        std::vector< std::byte > ten{std::byte{10}};
        for (std::size_t i = 0; i < fractional_digits; ++i)
        {
            denominator = detail::le_unsigned_mult_raw(std::move(denominator), ten);
        }

        return fixed_float_from_rational(opt, negative, std::move(numerator), std::move(denominator), sle_int_unlimited(0), track_exactness);
    }

    inline float_result fixed_float_from_int_le(fixed_float_options float_opt, fixed_int_options int_opt, std::vector< std::byte > const& value, bool track_exactness = false)
    {
        auto input = le_int_fixed_to_unlimited(int_opt, value);
        return fixed_float_from_rational(float_opt, input.is_negative, std::move(input.data), raw_one(), sle_int_unlimited(0), track_exactness);
    }

    inline sle_int_unlimited fixed_float_high_exponent(unpacked_fixed_float const& value)
    {
        assert(value.kind == fixed_float_kind::finite);
        return signed_add_size(value.scale, raw_bit_width(value.significand) - 1);
    }

    inline bool fixed_float_magnitude_less(unpacked_fixed_float const& a, unpacked_fixed_float const& b)
    {
        auto a_high = fixed_float_high_exponent(a);
        auto b_high = fixed_float_high_exponent(b);
        if (!signed_equal(a_high, b_high))
        {
            return signed_less(a_high, b_high);
        }

        auto common_scale = signed_less(a.scale, b.scale) ? a.scale : b.scale;
        auto a_magnitude = shift_left_by_signed_distance(a.significand, signed_sub(a.scale, common_scale));
        auto b_magnitude = shift_left_by_signed_distance(b.significand, signed_sub(b.scale, common_scale));
        return detail::le_comp_less_raw(a_magnitude, b_magnitude);
    }

    inline float_result fixed_float_add_finite(fixed_float_options opt, unpacked_fixed_float a, unpacked_fixed_float b)
    {
        auto a_high = fixed_float_high_exponent(a);
        auto b_high = fixed_float_high_exponent(b);
        auto high_difference = signed_abs_difference(a_high, b_high);
        if (signed_greater_than_size(high_difference, fixed_float_significand_bits(opt) + 4))
        {
            auto const& dominant = signed_less(a_high, b_high) ? b : a;
            return fixed_float_from_rational(opt, dominant.negative, dominant.significand, raw_one(), dominant.scale);
        }

        auto common_scale = signed_less(a.scale, b.scale) ? a.scale : b.scale;
        auto a_magnitude = shift_left_by_signed_distance(std::move(a.significand), signed_sub(a.scale, common_scale));
        auto b_magnitude = shift_left_by_signed_distance(std::move(b.significand), signed_sub(b.scale, common_scale));

        bool negative = false;
        std::vector< std::byte > magnitude;
        if (a.negative == b.negative)
        {
            negative = a.negative;
            magnitude = detail::unlimited_int_unsigned_add_le_raw(std::move(a_magnitude), std::move(b_magnitude));
        }
        else if (detail::le_comp_less_raw(a_magnitude, b_magnitude))
        {
            negative = b.negative;
            magnitude = detail::unlimited_int_unsigned_sub_le_raw(std::move(b_magnitude), std::move(a_magnitude));
        }
        else
        {
            negative = a.negative;
            magnitude = detail::unlimited_int_unsigned_sub_le_raw(std::move(a_magnitude), std::move(b_magnitude));
        }
        if (raw_is_zero(magnitude))
        {
            return fixed_float_zero(opt, false);
        }

        return fixed_float_from_rational(opt, negative, std::move(magnitude), raw_one(), std::move(common_scale));
    }

    inline float_result fixed_float_add_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        auto lhs = unpack_fixed_float(opt, a);
        auto rhs = unpack_fixed_float(opt, b);

        if (lhs.kind == fixed_float_kind::nan || rhs.kind == fixed_float_kind::nan)
        {
            return fixed_float_nan(opt);
        }
        if (lhs.kind == fixed_float_kind::infinity || rhs.kind == fixed_float_kind::infinity)
        {
            if (lhs.kind == fixed_float_kind::infinity && rhs.kind == fixed_float_kind::infinity && lhs.negative != rhs.negative)
            {
                return fixed_float_nan(opt);
            }
            return fixed_float_infinity(opt, lhs.kind == fixed_float_kind::infinity ? lhs.negative : rhs.negative);
        }
        if (lhs.kind == fixed_float_kind::zero && rhs.kind == fixed_float_kind::zero)
        {
            return fixed_float_zero(opt, lhs.negative && rhs.negative);
        }
        if (lhs.kind == fixed_float_kind::zero)
        {
            return {std::move(b), false};
        }
        if (rhs.kind == fixed_float_kind::zero)
        {
            return {std::move(a), false};
        }

        return fixed_float_add_finite(opt, std::move(lhs), std::move(rhs));
    }

    inline float_result fixed_float_sub_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        set_bit(b, opt.bits - 1, !raw_get_bit(b, opt.bits - 1));
        return fixed_float_add_le(opt, std::move(a), std::move(b));
    }

    inline float_result fixed_float_mul_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        auto lhs = unpack_fixed_float(opt, a);
        auto rhs = unpack_fixed_float(opt, b);
        bool negative = lhs.negative != rhs.negative;

        if (lhs.kind == fixed_float_kind::nan || rhs.kind == fixed_float_kind::nan)
        {
            return fixed_float_nan(opt);
        }
        if ((lhs.kind == fixed_float_kind::infinity && rhs.kind == fixed_float_kind::zero) || (lhs.kind == fixed_float_kind::zero && rhs.kind == fixed_float_kind::infinity))
        {
            return fixed_float_nan(opt);
        }
        if (lhs.kind == fixed_float_kind::infinity || rhs.kind == fixed_float_kind::infinity)
        {
            return fixed_float_infinity(opt, negative);
        }
        if (lhs.kind == fixed_float_kind::zero || rhs.kind == fixed_float_kind::zero)
        {
            return fixed_float_zero(opt, negative);
        }

        auto magnitude = detail::le_unsigned_mult_raw(std::move(lhs.significand), std::move(rhs.significand));
        auto scale = signed_add(std::move(lhs.scale), std::move(rhs.scale));
        return fixed_float_from_rational(opt, negative, std::move(magnitude), raw_one(), std::move(scale));
    }

    inline float_result fixed_float_div_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        auto lhs = unpack_fixed_float(opt, a);
        auto rhs = unpack_fixed_float(opt, b);
        bool negative = lhs.negative != rhs.negative;

        if (lhs.kind == fixed_float_kind::nan || rhs.kind == fixed_float_kind::nan)
        {
            return fixed_float_nan(opt);
        }
        if ((lhs.kind == fixed_float_kind::infinity && rhs.kind == fixed_float_kind::infinity) || (lhs.kind == fixed_float_kind::zero && rhs.kind == fixed_float_kind::zero))
        {
            return fixed_float_nan(opt);
        }
        if (lhs.kind == fixed_float_kind::infinity || rhs.kind == fixed_float_kind::zero)
        {
            return fixed_float_infinity(opt, negative);
        }
        if (lhs.kind == fixed_float_kind::zero || rhs.kind == fixed_float_kind::infinity)
        {
            return fixed_float_zero(opt, negative);
        }

        auto scale = signed_sub(std::move(lhs.scale), std::move(rhs.scale));
        return fixed_float_from_rational(opt, negative, std::move(lhs.significand), std::move(rhs.significand), std::move(scale));
    }

    inline bool_result fixed_float_ieee_eq_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        auto lhs = unpack_fixed_float(opt, a);
        auto rhs = unpack_fixed_float(opt, b);
        if (lhs.kind == fixed_float_kind::nan || rhs.kind == fixed_float_kind::nan)
        {
            return {false, false};
        }
        if (lhs.kind == fixed_float_kind::zero && rhs.kind == fixed_float_kind::zero)
        {
            return {true, false};
        }
        if (lhs.kind != rhs.kind || lhs.negative != rhs.negative)
        {
            return {false, false};
        }
        if (lhs.kind == fixed_float_kind::infinity)
        {
            return {true, false};
        }
        return {signed_equal(lhs.scale, rhs.scale) && le_comp_eq(lhs.significand, rhs.significand), false};
    }

    inline bool_result fixed_float_ieee_ne_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        auto result = fixed_float_ieee_eq_le(opt, std::move(a), std::move(b));
        return {!result.result, result.result_is_undefined};
    }

    inline bool_result fixed_float_ieee_lt_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        auto lhs = unpack_fixed_float(opt, a);
        auto rhs = unpack_fixed_float(opt, b);
        if (lhs.kind == fixed_float_kind::nan || rhs.kind == fixed_float_kind::nan)
        {
            return {false, false};
        }
        if (lhs.kind == fixed_float_kind::zero && rhs.kind == fixed_float_kind::zero)
        {
            return {false, false};
        }
        if (lhs.negative != rhs.negative)
        {
            return {lhs.negative, false};
        }
        if (lhs.kind == fixed_float_kind::infinity || rhs.kind == fixed_float_kind::infinity)
        {
            bool result = lhs.kind == fixed_float_kind::infinity ? lhs.negative && rhs.kind != fixed_float_kind::infinity : !rhs.negative;
            return {result, false};
        }
        if (lhs.kind == fixed_float_kind::zero)
        {
            return {!rhs.negative, false};
        }
        if (rhs.kind == fixed_float_kind::zero)
        {
            return {lhs.negative, false};
        }

        bool magnitude_less = fixed_float_magnitude_less(lhs, rhs);
        return {lhs.negative ? !magnitude_less && !fixed_float_ieee_eq_le(opt, std::move(a), std::move(b)).result : magnitude_less, false};
    }

    inline bool_result fixed_float_ieee_gt_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return fixed_float_ieee_lt_le(opt, std::move(b), std::move(a));
    }

    inline bool_result fixed_float_ieee_ge_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        auto lhs = unpack_fixed_float(opt, a);
        auto rhs = unpack_fixed_float(opt, b);
        if (lhs.kind == fixed_float_kind::nan || rhs.kind == fixed_float_kind::nan)
        {
            return {false, false};
        }
        return {!fixed_float_ieee_lt_le(opt, std::move(a), std::move(b)).result, false};
    }

    inline std::vector< std::byte > fixed_float_canonicalize_nan_le(fixed_float_options opt, std::vector< std::byte > value)
    {
        value = fixed_float_clear_padding_bits_le(opt, std::move(value));
        if (unpack_fixed_float(opt, value).kind == fixed_float_kind::nan)
        {
            return fixed_float_nan(opt).data_bytes;
        }
        return value;
    }

    inline int fixed_float_qux_compare_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        a = fixed_float_canonicalize_nan_le(opt, std::move(a));
        b = fixed_float_canonicalize_nan_le(opt, std::move(b));
        assert(a.size() == b.size());

        auto lhs = unpack_fixed_float(opt, a);
        auto rhs = unpack_fixed_float(opt, b);

        if (lhs.kind == fixed_float_kind::nan)
        {
            return rhs.kind == fixed_float_kind::nan ? 0 : 1;
        }
        if (rhs.kind == fixed_float_kind::nan)
        {
            return -1;
        }

        if (lhs.negative != rhs.negative)
        {
            return lhs.negative ? -1 : 1;
        }

        auto magnitude_rank = [](fixed_float_kind kind) -> int {
            switch (kind)
            {
            case fixed_float_kind::zero: return 0;
            case fixed_float_kind::finite: return 1;
            case fixed_float_kind::infinity: return 2;
            case fixed_float_kind::nan: throw quxlang::compiler_bug("nan in non-nan float ordering");
            }
            throw quxlang::compiler_bug("unknown fixed float kind");
        };

        int lhs_rank = magnitude_rank(lhs.kind);
        int rhs_rank = magnitude_rank(rhs.kind);
        if (lhs_rank != rhs_rank)
        {
            bool lhs_has_lower_magnitude = lhs_rank < rhs_rank;
            return lhs.negative == lhs_has_lower_magnitude ? 1 : -1;
        }

        if (lhs.kind != fixed_float_kind::finite)
        {
            return 0;
        }

        bool lhs_magnitude_less = fixed_float_magnitude_less(lhs, rhs);
        bool rhs_magnitude_less = fixed_float_magnitude_less(rhs, lhs);
        if (!lhs_magnitude_less && !rhs_magnitude_less)
        {
            return 0;
        }
        return lhs.negative == lhs_magnitude_less ? 1 : -1;
    }

    inline bool_result fixed_float_qux_eq_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return {fixed_float_qux_compare_le(opt, std::move(a), std::move(b)) == 0, false};
    }

    inline bool_result fixed_float_qux_lt_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return {fixed_float_qux_compare_le(opt, std::move(a), std::move(b)) < 0, false};
    }

    inline bool_result fixed_float_qux_ge_le(fixed_float_options opt, std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return {fixed_float_qux_compare_le(opt, std::move(a), std::move(b)) >= 0, false};
    }

} // namespace quxlang::bytemath

#endif // QUXLANG_FIXED_BYTEMATH_H
