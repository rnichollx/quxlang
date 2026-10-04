#include "pidigits.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>

big_integer::big_integer(std::int64_t value) : negative(value < 0)
{
    std::uint64_t magnitude = static_cast< std::uint64_t >(value);
    if (negative)
    {
        magnitude = std::uint64_t(0) - magnitude;
    }
    while (magnitude)
    {
        limbs.push_back(static_cast< std::uint32_t >(magnitude % 1000000000));
        magnitude /= 1000000000;
    }
}

void big_integer::normalize()
{
    while (!limbs.empty() && limbs.back() == 0)
    {
        limbs.pop_back();
    }
    if (limbs.empty())
    {
        negative = false;
    }
}

std::int32_t big_integer::compare(big_integer const& other) const
{
    if (limbs.size() != other.limbs.size())
    {
        return limbs.size() < other.limbs.size() ? -1 : 1;
    }
    for (std::size_t i = limbs.size(); i != 0; --i)
    {
        if (limbs[i - 1] != other.limbs[i - 1])
        {
            return limbs[i - 1] < other.limbs[i - 1] ? -1 : 1;
        }
    }
    return 0;
}

void big_integer::add_magnitude(big_integer const& other)
{
    std::size_t count = std::max(limbs.size(), other.limbs.size());
    limbs.resize(count);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        std::uint64_t sum = limbs[i] + carry;
        if (i < other.limbs.size())
        {
            sum += other.limbs[i];
        }
        limbs[i] = static_cast< std::uint32_t >(sum % 1000000000);
        carry = sum / 1000000000;
    }
    if (carry)
    {
        limbs.push_back(static_cast< std::uint32_t >(carry));
    }
}

void big_integer::subtract_magnitude(big_integer const& other)
{
    std::uint64_t borrow = 0;
    for (std::size_t i = 0; i < limbs.size(); ++i)
    {
        std::uint64_t subtract = borrow;
        if (i < other.limbs.size())
        {
            subtract += other.limbs[i];
        }
        std::uint64_t digit = limbs[i];
        if (digit < subtract)
        {
            digit += 1000000000;
            borrow = 1;
        }
        else
        {
            borrow = 0;
        }
        limbs[i] = static_cast< std::uint32_t >(digit - subtract);
    }
    normalize();
}

void big_integer::combine(big_integer const& other, bool other_negative)
{
    if (negative == other_negative)
    {
        add_magnitude(other);
        return;
    }
    if (compare(other) >= 0)
    {
        subtract_magnitude(other);
        return;
    }
    big_integer replacement = other;
    replacement.negative = other_negative;
    replacement.subtract_magnitude(*this);
    *this = std::move(replacement);
}

void big_integer::add(big_integer const& other)
{
    combine(other, other.negative);
}
void big_integer::subtract(big_integer const& other)
{
    combine(other, !other.negative);
}

void big_integer::multiply(std::uint32_t factor)
{
    if (factor == 0)
    {
        limbs.clear();
        negative = false;
        return;
    }
    std::uint64_t carry = 0;
    for (std::uint32_t& limb : limbs)
    {
        std::uint64_t product = static_cast< std::uint64_t >(limb) * factor + carry;
        limb = static_cast< std::uint32_t >(product % 1000000000);
        carry = product / 1000000000;
    }
    while (carry)
    {
        limbs.push_back(static_cast< std::uint32_t >(carry % 1000000000));
        carry /= 1000000000;
    }
}

std::int32_t big_integer::quotient(big_integer const& divisor) const
{
    if (divisor.limbs.empty())
    {
        throw std::domain_error("zero divisor");
    }
    bool result_negative = negative != divisor.negative;
    if (compare(divisor) < 0)
    {
        return result_negative && !limbs.empty() ? -1 : 0;
    }
    if (limbs.size() > divisor.limbs.size() + 2)
    {
        throw std::overflow_error("quotient");
    }
    std::size_t discarded = divisor.limbs.size() > 2 ? divisor.limbs.size() - 2 : 0;
    __uint128_t numerator = 0, denominator = 0;
    for (std::size_t i = limbs.size(); i > discarded; --i)
    {
        numerator = numerator * 1000000000 + limbs[i - 1];
    }
    for (std::size_t i = divisor.limbs.size(); i > discarded; --i)
    {
        denominator = denominator * 1000000000 + divisor.limbs[i - 1];
    }
    __uint128_t estimate = numerator / denominator;
    if (estimate > std::numeric_limits< std::uint32_t >::max())
    {
        throw std::overflow_error("quotient");
    }
    std::uint32_t result = static_cast< std::uint32_t >(estimate);
    big_integer product = divisor;
    product.negative = false;
    product.multiply(result);
    while (product.compare(*this) > 0)
    {
        --result;
        product.subtract_magnitude(divisor);
    }
    if (result_negative && product.compare(*this) != 0)
    {
        ++result;
    }
    if (result_negative)
    {
        if (result > UINT32_C(2147483648))
        {
            throw std::overflow_error("quotient");
        }
        return result == UINT32_C(2147483648) ? std::numeric_limits< std::int32_t >::min() : -static_cast< std::int32_t >(result);
    }
    if (result > std::numeric_limits< std::int32_t >::max())
    {
        throw std::overflow_error("quotient");
    }
    return static_cast< std::int32_t >(result);
}

std::int32_t extract_digit(big_integer const& q, big_integer const& r, big_integer const& t, std::uint32_t position)
{
    big_integer numerator = q;
    numerator.multiply(position);
    numerator.add(r);
    return numerator.quotient(t);
}

/** Generates decimal pi digits with the sequential linear-fractional spigot algorithm. */
int main(int argc, char** argv)
{
    std::size_t n = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 30;
    big_integer q(1), r, t(1);
    std::uint32_t term = 1;
    for (std::size_t produced = 0; produced < n;)
    {
        std::int32_t first = extract_digit(q, r, t, 3);
        std::int32_t second = extract_digit(q, r, t, 4);
        if (first == second)
        {
            std::putchar('0' + first);
            ++produced;
            if (produced % 10 == 0 || produced == n)
            {
                if (produced % 10)
                {
                    for (std::size_t i = produced % 10; i < 10; ++i)
                    {
                        std::putchar(' ');
                    }
                }
                std::printf("\t:%zu\n", produced);
            }
            big_integer removed = t;
            removed.multiply(static_cast< std::uint32_t >(first));
            r.subtract(removed);
            r.multiply(10);
            q.multiply(10);
        }
        else
        {
            big_integer twice_q = q;
            twice_q.multiply(2);
            r.add(twice_q);
            std::uint32_t factor = term * 2 + 1;
            r.multiply(factor);
            q.multiply(term);
            t.multiply(factor);
            ++term;
        }
    }
    return std::ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}
