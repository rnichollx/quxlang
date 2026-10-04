#include "pidigits.h"
#include <stdio.h>
#include <stdlib.h>

void big_integer_reserve(big_integer* value, size_t count)
{
    if (count <= value->capacity)
    {
        return;
    }
    size_t capacity = value->capacity ? value->capacity * 2 : 4;
    if (capacity < count)
    {
        capacity = count;
    }
    uint32_t* limbs = realloc(value->limbs, capacity * sizeof(uint32_t));
    if (!limbs)
    {
        abort();
    }
    value->limbs = limbs;
    value->capacity = capacity;
}

big_integer big_integer_copy(big_integer const* value)
{
    big_integer result = {0};
    big_integer_reserve(&result, value->size);
    for (size_t i = 0; i < value->size; ++i)
    {
        result.limbs[i] = value->limbs[i];
    }
    result.size = value->size;
    result.negative = value->negative;
    return result;
}

int32_t big_integer_compare(big_integer const* left, big_integer const* right)
{
    if (left->size != right->size)
    {
        return left->size < right->size ? -1 : 1;
    }
    for (size_t i = left->size; i != 0; --i)
    {
        if (left->limbs[i - 1] != right->limbs[i - 1])
        {
            return left->limbs[i - 1] < right->limbs[i - 1] ? -1 : 1;
        }
    }
    return 0;
}

void big_integer_normalize(big_integer* value)
{
    while (value->size && value->limbs[value->size - 1] == 0)
    {
        --value->size;
    }
    if (!value->size)
    {
        value->negative = false;
    }
}

void big_integer_add_magnitude(big_integer* value, big_integer const* other)
{
    size_t count = value->size > other->size ? value->size : other->size;
    big_integer_reserve(value, count + 1);
    uint64_t carry = 0;
    for (size_t i = 0; i < count; ++i)
    {
        uint64_t sum = carry;
        if (i < value->size)
        {
            sum += value->limbs[i];
        }
        if (i < other->size)
        {
            sum += other->limbs[i];
        }
        value->limbs[i] = (uint32_t)(sum % 1000000000);
        carry = sum / 1000000000;
    }
    value->size = count;
    if (carry)
    {
        value->limbs[value->size++] = (uint32_t)carry;
    }
}

void big_integer_subtract_magnitude(big_integer* value, big_integer const* other)
{
    uint64_t borrow = 0;
    for (size_t i = 0; i < value->size; ++i)
    {
        uint64_t subtract = borrow;
        if (i < other->size)
        {
            subtract += other->limbs[i];
        }
        uint64_t digit = value->limbs[i];
        if (digit < subtract)
        {
            digit += 1000000000;
            borrow = 1;
        }
        else
        {
            borrow = 0;
        }
        value->limbs[i] = (uint32_t)(digit - subtract);
    }
    big_integer_normalize(value);
}

void big_integer_combine(big_integer* value, big_integer const* other, bool negative)
{
    if (value->negative == negative)
    {
        big_integer_add_magnitude(value, other);
        return;
    }
    if (big_integer_compare(value, other) >= 0)
    {
        big_integer_subtract_magnitude(value, other);
        return;
    }
    big_integer replacement = big_integer_copy(other);
    replacement.negative = negative;
    big_integer_subtract_magnitude(&replacement, value);
    free(value->limbs);
    *value = replacement;
}

void big_integer_multiply(big_integer* value, uint32_t factor)
{
    big_integer_reserve(value, value->size + 2);
    uint64_t carry = 0;
    for (size_t i = 0; i < value->size; ++i)
    {
        uint64_t product = (uint64_t)value->limbs[i] * factor + carry;
        value->limbs[i] = (uint32_t)(product % 1000000000);
        carry = product / 1000000000;
    }
    while (carry)
    {
        value->limbs[value->size++] = (uint32_t)(carry % 1000000000);
        carry /= 1000000000;
    }
    big_integer_normalize(value);
}

int32_t big_integer_quotient(big_integer const* value, big_integer const* divisor)
{
    if (!divisor->size)
    {
        abort();
    }
    bool negative = value->negative != divisor->negative;
    if (big_integer_compare(value, divisor) < 0)
    {
        return negative && value->size ? -1 : 0;
    }
    if (value->size > divisor->size + 2)
    {
        abort();
    }
    size_t discarded = divisor->size > 2 ? divisor->size - 2 : 0;
    __uint128_t numerator = 0;
    __uint128_t denominator = 0;
    for (size_t i = value->size; i > discarded; --i)
    {
        numerator = numerator * 1000000000 + value->limbs[i - 1];
    }
    for (size_t i = divisor->size; i > discarded; --i)
    {
        denominator = denominator * 1000000000 + divisor->limbs[i - 1];
    }
    __uint128_t estimate = numerator / denominator;
    if (estimate > UINT32_MAX)
    {
        abort();
    }
    uint32_t quotient = (uint32_t)estimate;
    big_integer product = big_integer_copy(divisor);
    product.negative = false;
    big_integer_multiply(&product, quotient);
    while (big_integer_compare(&product, value) > 0)
    {
        --quotient;
        big_integer_subtract_magnitude(&product, divisor);
    }
    if (negative && big_integer_compare(&product, value) != 0)
    {
        ++quotient;
    }
    free(product.limbs);
    if (negative)
    {
        if (quotient > UINT32_C(2147483648))
        {
            abort();
        }
        return quotient == UINT32_C(2147483648) ? INT32_MIN : -(int32_t)quotient;
    }
    if (quotient > INT32_MAX)
    {
        abort();
    }
    return (int32_t)quotient;
}

int32_t extract_digit(big_integer const* q, big_integer const* r, big_integer const* t, uint32_t position)
{
    big_integer numerator = big_integer_copy(q);
    big_integer_multiply(&numerator, position);
    big_integer_combine(&numerator, r, r->negative);
    int32_t digit = big_integer_quotient(&numerator, t);
    free(numerator.limbs);
    return digit;
}

/** Generates decimal pi digits with the sequential linear-fractional spigot algorithm. */
int main(int argc, char** argv)
{
    size_t n = argc > 1 ? (size_t)strtoull(argv[1], NULL, 10) : 30;
    big_integer q = {0}, r = {0}, t = {0};
    big_integer_reserve(&q, 1);
    q.limbs[0] = 1;
    q.size = 1;
    t = big_integer_copy(&q);
    uint32_t term = 1;
    for (size_t produced = 0; produced < n;)
    {
        int32_t first = extract_digit(&q, &r, &t, 3);
        int32_t second = extract_digit(&q, &r, &t, 4);
        if (first == second)
        {
            putchar('0' + first);
            ++produced;
            if (produced % 10 == 0 || produced == n)
            {
                if (produced % 10)
                {
                    for (size_t i = produced % 10; i < 10; ++i)
                    {
                        putchar(' ');
                    }
                }
                printf("\t:%zu\n", produced);
            }
            big_integer removed = big_integer_copy(&t);
            big_integer_multiply(&removed, (uint32_t)first);
            big_integer_combine(&r, &removed, !removed.negative);
            free(removed.limbs);
            big_integer_multiply(&r, 10);
            big_integer_multiply(&q, 10);
        }
        else
        {
            big_integer twice_q = big_integer_copy(&q);
            big_integer_multiply(&twice_q, 2);
            big_integer_combine(&r, &twice_q, twice_q.negative);
            free(twice_q.limbs);
            uint32_t factor = term * 2 + 1;
            big_integer_multiply(&r, factor);
            big_integer_multiply(&q, term);
            big_integer_multiply(&t, factor);
            ++term;
        }
    }
    free(q.limbs);
    free(r.limbs);
    free(t.limbs);
    return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}
