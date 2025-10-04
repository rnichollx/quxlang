// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/bytemath.hpp"

#include <stdexcept>

namespace quxlang::bytemath
{
    std::pair< std::uint8_t, std::uint8_t > bytewise_add(std::uint8_t a, std::uint8_t b, std::uint8_t c)
    {
        std::uint16_t sum = std::uint16_t(0) + a + b + c;

        std::uint8_t carry = sum >> 8;
        std::uint8_t result = sum & 0xFF;

        return {result, carry};
    }
    std::uint8_t le_get(std::vector< std::byte > const& data, std::size_t index)
    {
        if (index >= data.size())
        {
            return 0;
        }

        return static_cast< std::uint8_t >(data[index]);
    }
    void le_trim(std::vector< std::byte >& v)
    {
        while (v.size() > 1 && v.back() == std::byte{0})
        {
            v.pop_back();
        }

        if (v.size() == 0)
        {
            v.push_back(std::byte{0});
        }
    }
    std::vector< std::byte > unlimited_int_unsigned_add_le(std::vector< std::byte > a, std::vector< std::byte > b)
    {
        std::uint8_t carry = 0;
        std::uint8_t result = 0;

        for (std::size_t index = 0; index < std::max(a.size(), b.size()); index++)
        {
            std::uint8_t a_byte = le_get(a, index);
            std::uint8_t b_byte = le_get(b, index);

            std::tie(result, carry) = bytewise_add(a_byte, b_byte, carry);

            if (index < a.size())
            {
                a[index] = static_cast< std::byte >(result);
            }
            else
            {
                a.push_back(static_cast< std::byte >(result));
            }
        }

        if (carry)
        {
            a.push_back(static_cast< std::byte >(carry));
        }

        return std::move(a);
    }
    std::vector< std::byte > unlimited_int_unsigned_sub_le(std::vector< std::byte > a, std::vector< std::byte > b)
    {
        std::uint8_t borrow = 0;

        for (std::size_t index = 0; index < a.size(); ++index)
        {
            std::uint16_t ai = static_cast< std::uint8_t >(a[index]);
            std::uint16_t bi = le_get(b, index);
            std::uint16_t tmp = 0;

            if (ai < bi + borrow)
            {
                tmp = ai + 0x100 - bi - borrow;
                borrow = 1;
            }
            else
            {
                tmp = ai - bi - borrow;
                borrow = 0;
            }

            a[index] = static_cast< std::byte >(tmp & 0xFF);
        }

        le_trim(a);
        return std::move(a);
    }
    bool le_comp_less(std::vector< std::byte > const& a, std::vector< std::byte > const& b)
    {
        std::size_t maxlen = std::max(a.size(), b.size());

        for (std::size_t i = maxlen - 1; true; --i)
        {
            std::uint8_t ai = le_get(a, i);
            std::uint8_t bi = le_get(b, i);

            if (ai < bi)
            {
                return true;
            }
            else if (ai > bi)
            {
                return false;
            }

            if (i == 0)
            {
                break;
            }
        }

        return false;
    }
    std::vector< std::byte > le_unsigned_mult(std::vector< std::byte > a, std::vector< std::byte > b)
    {
        std::vector< std::byte > result(a.size() + b.size(), std::byte{0});

        for (std::size_t i = 0; i < a.size(); ++i)
        {
            std::uint16_t carry = 0;
            std::uint8_t ai = le_get(a, i);

            for (std::size_t j = 0; j < b.size(); ++j)
            {
                std::size_t k = i + j;
                std::uint16_t prod = std::uint16_t(ai) * std::uint16_t(le_get(b, j)) + std::uint16_t(static_cast< std::uint8_t >(result[k])) + carry;

                result[k] = static_cast< std::byte >(prod & 0xFF);
                carry = prod >> 8;
            }

            result[i + b.size()] = static_cast< std::byte >(carry);
        }

        le_trim(result);
        return result;
    }
    std::pair< std::vector< std::byte >, std::vector< std::byte > > le_unsigned_divmod(std::vector< std::byte > a, std::vector< std::byte > b)
    {
        le_trim(a);
        le_trim(b);

        // division by zero -> return zero quotient, original a as rem
        if (b.size() == 0 || (b.size() == 1 && b[0] == std::byte{0}))
        {
            return {std::vector< std::byte >{std::byte{0}}, std::move(a)};
        }

        if (!le_comp_less(a, b))
        {
            std::size_t n = a.size();
            std::size_t m = b.size();
            std::size_t t = (n >= m ? n - m : 0);

            std::vector< std::uint8_t > q_big(t + 1, 0);

            for (std::size_t shift = t + 1; shift-- > 0;)
            {
                // binary search d in [0..255]
                std::uint8_t lo = 0, hi = 255, best = 0;
                while (lo <= hi)
                {
                    std::uint8_t mid = std::uint8_t((lo + hi) >> 1);

                    // compute b * mid
                    std::vector< std::byte > prod;
                    prod.reserve(b.size() + 1);
                    std::uint16_t carry = 0;
                    for (std::size_t j = 0; j < m; ++j)
                    {
                        std::uint16_t v = std::uint16_t(le_get(b, j)) * mid + carry;
                        prod.push_back(static_cast< std::byte >(v & 0xFF));
                        carry = v >> 8;
                    }
                    if (carry)
                        prod.push_back(static_cast< std::byte >(carry));

                    // shift by 'shift' bytes
                    std::vector< std::byte > shifted(shift, std::byte{0});
                    shifted.insert(shifted.end(), prod.begin(), prod.end());
                    le_trim(shifted);

                    // compare shifted <= a ?
                    if (!le_comp_less(a, shifted))
                    {
                        best = mid;
                        lo = mid + 1;
                    }
                    else
                    {
                        if (mid == 0)
                            break;
                        hi = mid - 1;
                    }
                }

                if (best)
                {
                    // subtract best * b << shift from a
                    std::vector< std::byte > prod;
                    prod.reserve(b.size() + 1);
                    std::uint16_t carry = 0;
                    for (std::size_t j = 0; j < m; ++j)
                    {
                        std::uint16_t v = std::uint16_t(le_get(b, j)) * best + carry;
                        prod.push_back(static_cast< std::byte >(v & 0xFF));
                        carry = v >> 8;
                    }
                    if (carry)
                        prod.push_back(static_cast< std::byte >(carry));

                    std::vector< std::byte > to_sub(shift, std::byte{0});
                    to_sub.insert(to_sub.end(), prod.begin(), prod.end());
                    a = unlimited_int_unsigned_sub_le(std::move(a), std::move(to_sub));
                }

                q_big[t - shift] = best;
            }

            // build little-endian quotient
            std::vector< std::byte > q_le;
            q_le.reserve(q_big.size());
            for (std::size_t i = 0; i < q_big.size(); ++i)
            {
                q_le.push_back(static_cast< std::byte >(q_big[q_big.size() - 1 - i]));
            }
            le_trim(q_le);

            return {std::move(q_le), std::move(a)};
        }
        else
        {
            // a < b
            return {std::vector< std::byte >{std::byte{0}}, std::move(a)};
        }
    }
    std::vector< std::byte > le_unsigned_div(std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return le_unsigned_divmod(std::move(a), std::move(b)).first;
    }
    std::vector< std::byte > le_unsigned_mod(std::vector< std::byte > a, std::vector< std::byte > b)
    {
        return le_unsigned_divmod(std::move(a), std::move(b)).second;
    }
    std::vector< std::byte > le_shift_down(std::vector< std::byte > value, std::size_t shift)
    {
        std::size_t byte_shift = shift / 8;
        std::size_t bit_shift = shift % 8;

        value.erase(value.begin(), value.begin() + std::min(byte_shift, value.size()));

        if (bit_shift > 0)
        {
            std::size_t num_bytes = value.size();
            if (num_bytes != 0)
            {
                for (std::size_t i = 0; i < num_bytes - 1; ++i)
                {
                    value[i] = static_cast< std::byte >((static_cast< std::uint8_t >(value[i]) >> bit_shift) | (static_cast< std::uint8_t >(value[i + 1]) << (8 - bit_shift)));
                }

                value[num_bytes - 1] = static_cast< std::byte >(static_cast< std::uint8_t >(value[num_bytes - 1]) >> bit_shift);
            }
        }

        if (value.empty())
        {
            value.push_back(std::byte{0});
        }

        return value;
    }
    std::vector< std::byte > le_shift_up(std::vector< std::byte > value, std::size_t shift)
    {
        std::size_t byte_shift = shift / 8;
        std::size_t bit_shift = shift % 8;

        if (value.empty())
        {
            return {std::byte{0}};
        }

        value.insert(value.begin(), byte_shift, std::byte{0});

        if (bit_shift > 0)
        {
            std::size_t num_bytes = value.size();
            std::byte carry = std::byte{0};

            for (std::size_t i = 0; i < num_bytes; ++i)
            {
                std::uint8_t current = static_cast< std::uint8_t >(value[i]);
                std::uint8_t next_carry = (current >> (8 - bit_shift));
                std::uint8_t shifted = (current << bit_shift) | static_cast< std::uint8_t >(carry);

                value[i] = static_cast< std::byte >(shifted);
                carry = static_cast< std::byte >(next_carry);
            }

            if (carry != std::byte{0})
            {
                value.push_back(carry);
            }
        }

        return value;
    }
    std::string le_to_string(std::vector< std::byte > number)
    {
        std::string result;
        std::vector< std::byte > ten = {std::byte{10}};

        while (!le_comp_eq(number, {}))
        {
            auto mod_result = le_unsigned_divmod(std::move(number), ten);
            assert(mod_result.second.size() <= 1);
            assert(le_comp_less(mod_result.second, ten));

            std::uint8_t mod_val = le_get(mod_result.second, 0);
            assert(mod_val < 10);
            char digit = static_cast< char >(mod_val + '0');
            result.push_back(digit);
            number = std::move(mod_result.first);
        }
        std::reverse(result.begin(), result.end());
        if (result.empty())
        {
            result = "0";
        }
        return result;
    }
    std::vector< std::byte > string_to_le(std::string const& str)
    {
        std::vector< std::byte > result;
        std::vector< std::byte > ten = {std::byte{10}};
        for (std::size_t i = 0; i < str.size(); ++i)
        {
            char c = str[i];
            if (c < '0' || c > '9')
            {
                throw std::invalid_argument("Invalid character in string: " + std::string(1, c));
            }
            if (result.empty() && c == '0')
            {
                continue; // skip leading zeros
            }

            auto digit = u_to_le(static_cast< std::uint8_t >(c - '0'));

            result = le_unsigned_mult(result, ten);
            result = unlimited_int_unsigned_add_le(result, digit);
        }
        if (result.empty())
        {
            result.push_back(std::byte{0});
        }

        return result;
    }
    std::vector< std::byte > le_truncate(std::vector< std::byte > data, std::size_t size_in_bits)
    {

        std::size_t total_num_result_bytes = (size_in_bits + 7) / 8;
        std::size_t num_bits_in_last_byte = size_in_bits % 8;
        if (num_bits_in_last_byte == 0 && size_in_bits != 0)
        {
            num_bits_in_last_byte = 8;
        }
        if (data.size() > total_num_result_bytes)
        {
            data.resize(total_num_result_bytes);
        }

        if (total_num_result_bytes != 0 && data.size() == total_num_result_bytes && num_bits_in_last_byte != 8)
        {
            // Clear the bits above the specified size
            std::uint8_t mask = (1 << num_bits_in_last_byte) - 1;
            data[total_num_result_bytes - 1] &= static_cast< std::byte >(mask);
        }

        if (data.empty())
        {
            data.push_back(std::byte{0});
        }

        return data;
    }
    sle_int_unlimited unlimited_int_signed_add_le(sle_int_unlimited a, sle_int_unlimited b)
    {
        bool a_negative = a.is_negative;
        bool b_negative = b.is_negative;

        if (a_negative == b_negative)
        {
            auto result = unlimited_int_unsigned_add_le(std::move(a.data), std::move(b.data));
            return {std::move(result), a_negative};
        }
        else
        {
            if (le_comp_less(a.data, b.data))
            {
                auto result = unlimited_int_unsigned_sub_le(std::move(b.data), std::move(a.data));
                return {std::move(result), !b_negative};
            }
            else
            {
                auto result = unlimited_int_unsigned_sub_le(std::move(a.data), std::move(b.data));
                return {std::move(result), a_negative};
            }
        }
    }
    sle_int_unlimited unlimited_int_signed_sub_le(sle_int_unlimited a, sle_int_unlimited b)
    {
        b.is_negative = !b.is_negative;
        return unlimited_int_signed_add_le(std::move(a), std::move(b));
    }
    sle_int_unlimited le_signed_div(sle_int_unlimited a, sle_int_unlimited b)
    {
        auto result = le_unsigned_div(std::move(a.data), std::move(b.data));
        return sle_int_unlimited(std::move(result), a.is_negative != b.is_negative);
    }
    sle_int_unlimited le_signed_mult(sle_int_unlimited a, sle_int_unlimited b)
    {
        auto result = le_unsigned_mult(std::move(a.data), std::move(b.data));
        return sle_int_unlimited(std::move(result), a.is_negative != b.is_negative);
    }
} // namespace quxlang::bytemath