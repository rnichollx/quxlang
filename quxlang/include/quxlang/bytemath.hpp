// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_BYTEMATH_HEADER_GUARD
#define QUXLANG_BYTEMATH_HEADER_GUARD

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace quxlang::bytemath
{
    struct sle_int_unlimited;
    struct ule_int_unlimited;

    namespace detail
    {
        // Add the two numbers
        std::vector< std::byte > le_unsigned_add_raw(std::vector< std::byte > a, std::vector< std::byte > b);
        // multiply the two numbers
        std::vector< std::byte > le_unsigned_mult_raw(std::vector< std::byte > a, std::vector< std::byte > b);
        // divide the two numbers
        std::vector< std::byte > le_unsigned_div_raw(std::vector< std::byte > a, std::vector< std::byte > b);

        // subtract the two numbers, assuming a >= b
        std::vector< std::byte > le_unsigned_sub_raw(std::vector< std::byte > a, std::vector< std::byte > b);

        // compare two numbers a < b
        bool le_comp_less_raw(std::vector< std::byte > const& a, std::vector< std::byte > const& b);
    }

    inline bool le_comp_eq(std::vector< std::byte > const& a, std::vector< std::byte > const& b)
    {
        return !detail::le_comp_less_raw(a, b) && !detail::le_comp_less_raw(b, a);
    }

    // remainder of a / b
    namespace detail
    {
        std::vector< std::byte > le_unsigned_mod_raw(std::vector< std::byte > a, std::vector< std::byte > b);

        // Raw versions for all functions that operate on std::vector<std::byte>
        std::uint8_t le_get_raw(std::vector< std::byte > const& data, std::size_t index);
        void le_trim_raw(std::vector< std::byte >& v);
        std::vector< std::byte > unlimited_int_unsigned_add_le_raw(std::vector< std::byte > a, std::vector< std::byte > b);
        std::vector< std::byte > unlimited_int_unsigned_sub_le_raw(std::vector< std::byte > a, std::vector< std::byte > b);
        std::vector< std::byte > le_unsigned_mult_raw(std::vector< std::byte > a, std::vector< std::byte > b);
        std::pair< std::vector< std::byte >, std::vector< std::byte > > le_unsigned_divmod_raw(std::vector< std::byte > a, std::vector< std::byte > b);
        std::vector< std::byte > le_unsigned_div_raw(std::vector< std::byte > a, std::vector< std::byte > b);
        std::vector< std::byte > le_shift_down_raw(std::vector< std::byte > value, std::size_t shift);
        std::vector< std::byte > le_shift_up_raw(std::vector< std::byte > value, std::size_t shift);
        std::string le_to_string_raw(std::vector< std::byte > number);
        std::vector< std::byte > string_to_le_raw(std::string const& str);
        std::vector< std::byte > le_truncate_raw(std::vector< std::byte > data, std::size_t size_in_bits);

        template < typename UInt >
        inline std::pair< UInt, bool > le_to_u_raw(std::vector< std::byte > const& data)
        {
            static_assert(std::is_integral_v< UInt >, "Int must be an integral type");
            constexpr UInt maxval = std::numeric_limits< UInt >::max();

            UInt v = 0;
            for (std::size_t i = data.size(); i-- > 0;)
            {
                std::uint8_t b = static_cast< std::uint8_t >(data[i]);
                if (v > (maxval >> 8))
                {
                    return {0, false};
                }
                v <<= 8;
                if (static_cast< UInt >(b) > (maxval - v))
                {
                    return {0, false};
                }
                v += static_cast< UInt >(b);
            }
            return {static_cast< UInt >(v), true};
        }

        template < typename UInt >
        inline std::vector< std::byte > u_to_le_raw(UInt value)
        {
            static_assert(std::is_integral_v< UInt >, "UInt must be an integral type");
            static_assert(std::is_unsigned_v< UInt >, "UInt must be an unsigned type");
            std::vector< std::byte > result;

            while (value != 0)
            {
                result.push_back(static_cast< std::byte >(value & 0xFF));
                value >>= 4;
                value >>= 4;
            }

            return result;
        }
    }

    std::pair< std::uint8_t, std::uint8_t > bytewise_add(std::uint8_t a, std::uint8_t b, std::uint8_t c);

    template < typename UInt >
    inline std::pair< UInt, bool > le_to_u(std::vector< std::byte > const& data)
    {
        static_assert(std::is_integral_v< UInt >, "Int must be an integral type");
        constexpr UInt maxval = std::numeric_limits< UInt >::max();

        UInt v = 0;
        // process most‐significant byte first
        for (std::size_t i = data.size(); i-- > 0;)
        {
            std::uint8_t b = static_cast< std::uint8_t >(data[i]);
            // will shifting left by 8 overflow?
            if (v > (maxval >> 8))
            {
                return {0, false};
            }
            v <<= 8;
            // will adding b overflow?
            if (static_cast< UInt >(b) > (maxval - v))
            {
                return {0, false};
            }
            v += static_cast< UInt >(b);
        }
        return {static_cast< UInt >(v), true};
    }

    template < typename UInt >
    std::vector< std::byte > u_to_le(UInt value)
    {
        static_assert(std::is_integral_v< UInt >, "UInt must be an integral type");
        static_assert(std::is_unsigned_v< UInt >, "UInt must be an unsigned type");
        std::vector< std::byte > result;

        while (value != 0)
        {
            result.push_back(static_cast< std::byte >(value & 0xFF));
            // Double shift for case where uint8 is used.
            value >>= 4;
            value >>= 4;
        }

        return result;
    }

    sle_int_unlimited unlimited_int_signed_add_le(sle_int_unlimited a, sle_int_unlimited b);

    sle_int_unlimited unlimited_int_signed_sub_le(sle_int_unlimited a, sle_int_unlimited b);

    sle_int_unlimited le_signed_div(sle_int_unlimited a, sle_int_unlimited b);




    struct ule_int_unlimited
    {
        std::vector< std::byte > data;

        ule_int_unlimited() : data{std::byte(0)}
        {
        }

        template < typename U >
        ule_int_unlimited(U value)
        {
            static_assert(std::is_integral_v< U >, "U must be an integral type");

            data = detail::u_to_le_raw(value);
        }
    };

    struct sle_int_unlimited
    {
        std::vector< std::byte > data;
        bool is_negative = false;

        sle_int_unlimited(std::vector< std::byte > data, bool is_negative = false) : data(std::move(data)), is_negative(is_negative)
        {
        }

        template < typename I >
        sle_int_unlimited(I value)
        {
            static_assert(std::numeric_limits< I >::radix == 2);

            if (value == 0)
            {
                data = {std::byte(0)};
                return;
            }
            if (value == 1)
            {
                data = {std::byte(1)};
                return;
            }
            if (value == 2)
            {
                data = {std::byte(2)};
                return;
            }
            if (value == -1)
            {
                data = {std::byte(1)};
                is_negative = true;
                return;
            }
            if (value < 0)
            {
                is_negative = true;
                sle_int_unlimited extra{-1};

                while (value > 0)
                {
                    // Cannot use bitwise operations for non-two's complement integers
                    // This is well-defined for C++11 onward even for sign-magnitude integers
                    if (value % 2 == -1)
                    {
                        this->data = detail::unlimited_int_unsigned_add_le_raw(std::move(this->data), extra.data);
                    }
                    extra.data = detail::le_shift_up_raw(extra.data, 1);

                    value = value / 2;
                }
            }
        }

        bool operator==(sle_int_unlimited const& other) const
        {
            return is_negative == other.is_negative && le_comp_eq(data, other.data);
        }

        bool operator!=(sle_int_unlimited const& other) const
        {
            return !(*this == other);
        }

        bool operator<(sle_int_unlimited const& other) const
        {
            if (is_negative != other.is_negative)
            {
                return is_negative;
            }
            if (is_negative)
            {
                return detail::le_comp_less_raw(other.data, data);
            }
            else
            {
                return detail::le_comp_less_raw(data, other.data);
            }
        }

        bool operator>(sle_int_unlimited const& other) const
        {
            if (is_negative != other.is_negative)
            {
                return !is_negative;
            }
            if (is_negative)
            {
                return detail::le_comp_less_raw(data, other.data);
            }
            else
            {
                return detail::le_comp_less_raw(other.data, data);
            }
        }

        bool operator<=(sle_int_unlimited const& other) const
        {
            return !(*this > other);
        }

        bool operator>=(sle_int_unlimited const& other) const
        {
            return !(*this < other);
        }

        template < typename I >
        std::pair< I, bool > to_int() &&
        {
            static sle_int_unlimited max_value = sle_int_unlimited(std::numeric_limits< I >::max());
            static sle_int_unlimited min_value = sle_int_unlimited(std::numeric_limits< I >::min());
            static sle_int_unlimited zero = sle_int_unlimited(0);

            detail::le_trim_raw(data);

            if (*this > max_value)
            {
                return {0, false};
            }
            if (*this < min_value)
            {
                return {0, false};
            }

            if (*this == zero)
            {
                return {0, true};
            }

            I result = 0;

            while (*this > 0)
            {
                result <<= 4;
                result += I(std::uint8_t(data[data.size() - 1]) & 0xF0);
                result <<= 4;
                result += I(std::uint8_t(data[data.size()]) & 0xF);

                data = detail::le_shift_down_raw(std::move(data), 8);
            }

            while (*this < 0)
            {
                result <<= 4;
                result -= I(std::uint8_t(data[data.size() - 1]) & 0xF0);
                result <<= 4;
                result -= I(std::uint8_t(data[data.size()]) & 0xF);

                data = detail::le_shift_down_raw(std::move(data), 8);
            }

            return {result, true};
        }

        template < typename I >
        std::pair< I, bool > to_int() const&
        {
            sle_int_unlimited this_copy = *this;
            return std::move(this_copy).to_int< I >();
        }
    };


} // namespace quxlang::bytemath

#endif // BYTEMATH_HPP
