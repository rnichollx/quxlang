#ifndef QUXLANG_BYTEMATH_HPP
#define QUXLANG_BYTEMATH_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace quxlang::bytemath
{
    struct le_sint;
    struct le_uint;

    namespace detail
    {
        // Add the two numbers
        std::vector< std::byte > le_unsigned_add(std::vector< std::byte > a, std::vector< std::byte > b);
        // multiply the two numbers
        std::vector< std::byte > le_unsigned_mult(std::vector< std::byte > a, std::vector< std::byte > b);
        // divide the two numbers
        std::vector< std::byte > le_unsigned_div(std::vector< std::byte > a, std::vector< std::byte > b);

        // subtract the two numbers, assuming a >= b
        std::vector< std::byte > le_unsigned_sub(std::vector< std::byte > a, std::vector< std::byte > b);
    } // namespace detail

    // compare two numbers a < b
    bool le_comp_less(std::vector< std::byte > const& a, std::vector< std::byte > const& b);

    inline bool le_comp_eq(std::vector< std::byte > const& a, std::vector< std::byte > const& b)
    {
        return !le_comp_less(a, b) && !le_comp_less(b, a);
    }

    // remainder of a / b
    std::vector< std::byte > le_unsigned_mod(std::vector< std::byte > a, std::vector< std::byte > b);

    std::uint8_t le_get(std::vector< std::byte > const& data, std::size_t index);

    std::pair< std::uint8_t, std::uint8_t > bytewise_add(std::uint8_t a, std::uint8_t b, std::uint8_t c);



    void le_trim(std::vector< std::byte >& v);

    std::vector< std::byte > le_unsigned_add(std::vector< std::byte > a, std::vector< std::byte > b);

    std::vector< std::byte > le_unsigned_sub(std::vector< std::byte > a, std::vector< std::byte > b);



    std::vector< std::byte > le_unsigned_mult(std::vector< std::byte > a, std::vector< std::byte > b);
    inline le_sint le_signed_mult(le_sint a, le_sint b);

    // helper: divmod via long division by byte-wise trial
    std::pair< std::vector< std::byte >, std::vector< std::byte > > le_unsigned_divmod(std::vector< std::byte > a, std::vector< std::byte > b);

    std::vector< std::byte > le_unsigned_div(std::vector< std::byte > a, std::vector< std::byte > b);

    std::vector< std::byte > le_unsigned_mod(std::vector< std::byte > a, std::vector< std::byte > b);

    std::vector< std::byte > le_shift_down(std::vector< std::byte > value, std::size_t shift);

    std::vector< std::byte > le_shift_up(std::vector< std::byte > value, std::size_t shift);

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

    le_sint le_signed_add(le_sint a, le_sint b);

    le_sint le_signed_sub(le_sint a, le_sint b);

    le_sint le_signed_div(le_sint a, le_sint b);



    std::string le_to_string(std::vector< std::byte > number);

    std::vector< std::byte > string_to_le(std::string const& str);

    std::vector< std::byte > le_truncate(std::vector< std::byte > data, std::size_t size_in_bits);

    struct le_uint
    {
        std::vector< std::byte > data;

        le_uint() : data{std::byte(0)}
        {
        }

        template < typename U >
        le_uint(U value)
        {
            static_assert(std::is_integral_v< U >, "U must be an integral type");

            data = u_to_le(value);
        }
    };

    struct le_sint
    {
        std::vector< std::byte > data;
        bool is_negative = false;

        le_sint(std::vector< std::byte > data, bool is_negative = false) : data(std::move(data)), is_negative(is_negative)
        {
        }

        template < typename I >
        le_sint(I value)
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
                le_sint extra{-1};

                while (value > 0)
                {
                    // Cannot use bitwise operations for non-two's complement integers
                    // This is well-defined for C++11 onward even for sign-magnitude integers
                    if (value % 2 == -1)
                    {
                        this->data = le_unsigned_add(std::move(this->data), extra.data);
                    }
                    extra.data = le_shift_up(extra.data, 1);

                    value = value / 2;
                }
            }
        }

        bool operator==(le_sint const& other) const
        {
            return is_negative == other.is_negative && le_comp_eq(data, other.data);
        }

        bool operator!=(le_sint const& other) const
        {
            return !(*this == other);
        }

        bool operator<(le_sint const& other) const
        {
            if (is_negative != other.is_negative)
            {
                return is_negative;
            }
            if (is_negative)
            {
                return le_comp_less(other.data, data);
            }
            else
            {
                return le_comp_less(data, other.data);
            }
        }

        bool operator>(le_sint const& other) const
        {
            if (is_negative != other.is_negative)
            {
                return !is_negative;
            }
            if (is_negative)
            {
                return le_comp_less(data, other.data);
            }
            else
            {
                return le_comp_less(other.data, data);
            }
        }

        bool operator<=(le_sint const& other) const
        {
            return !(*this > other);
        }

        bool operator>=(le_sint const& other) const
        {
            return !(*this < other);
        }

        template < typename I >
        std::pair< I, bool > to_int() &&
        {
            static le_sint max_value = le_sint(std::numeric_limits< I >::max());
            static le_sint min_value = le_sint(std::numeric_limits< I >::min());
            static le_sint zero = le_sint(0);

            le_trim(data);

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

                data = le_shift_down(std::move(data), 8);
            }

            while (*this < 0)
            {
                result <<= 4;
                result -= I(std::uint8_t(data[data.size() - 1]) & 0xF0);
                result <<= 4;
                result -= I(std::uint8_t(data[data.size()]) & 0xF);

                data = le_shift_down(std::move(data), 8);
            }

            return {result, true};
        }

        template < typename I >
        std::pair< I, bool > to_int() const&
        {
            le_sint this_copy = *this;
            return std::move(this_copy).to_int< I >();
        }
    };


} // namespace quxlang::bytemath

#endif // BYTEMATH_HPP
