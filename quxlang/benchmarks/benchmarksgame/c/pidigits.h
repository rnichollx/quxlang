#ifndef QUXLANG_BENCHMARKSGAME_C_PIDIGITS_H
#define QUXLANG_BENCHMARKSGAME_C_PIDIGITS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Owns a signed base-one-billion arbitrary-precision integer. */
typedef struct big_integer
{
    uint32_t* limbs;
    size_t size;
    size_t capacity;
    bool negative;
} big_integer;

/** Reserves space for at least count limbs. */
void big_integer_reserve(big_integer* value, size_t count);
/** Constructs a deep copy. */
big_integer big_integer_copy(big_integer const* value);
/** Compares unsigned magnitudes. */
int32_t big_integer_compare(big_integer const* left, big_integer const* right);
/** Removes leading zero limbs. */
void big_integer_normalize(big_integer* value);
/** Adds a magnitude. */
void big_integer_add_magnitude(big_integer* value, big_integer const* other);
/** Subtracts a magnitude no greater than value. */
void big_integer_subtract_magnitude(big_integer* value, big_integer const* other);
/** Adds other's magnitude with the selected sign. */
void big_integer_combine(big_integer* value, big_integer const* other, bool negative);
/** Multiplies by a nonnegative machine factor. */
void big_integer_multiply(big_integer* value, uint32_t factor);
/** Computes a signed floor quotient that fits int32_t. */
int32_t big_integer_quotient(big_integer const* value, big_integer const* divisor);
/** Extracts one tentative digit from a linear fractional transform. */
int32_t extract_digit(big_integer const* q, big_integer const* r, big_integer const* t, uint32_t position);
#endif
