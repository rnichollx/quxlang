#ifndef QUXLANG_BENCHMARKSGAME_CPP_PIDIGITS_HPP
#define QUXLANG_BENCHMARKSGAME_CPP_PIDIGITS_HPP
#include <cstdint>
#include <vector>

/** Owns a signed base-one-billion arbitrary-precision integer. */
class big_integer
{
    std::vector< std::uint32_t > limbs;
    bool negative = false;

    /** Removes leading zero limbs. */
    void normalize();
    /** Compares std::uint32_t magnitudes. */
    std::int32_t compare(big_integer const& other) const;
    /** Adds a magnitude. */
    void add_magnitude(big_integer const& other);
    /** Subtracts a magnitude no greater than this value. */
    void subtract_magnitude(big_integer const& other);
    /** Adds other's magnitude with the selected sign. */
    void combine(big_integer const& other, bool other_negative);

  public:
    /** Constructs an exact signed machine integer. */
    explicit big_integer(std::int64_t value = 0);
    /** Adds a signed arbitrary-precision value. */
    void add(big_integer const& other);
    /** Subtracts a signed arbitrary-precision value. */
    void subtract(big_integer const& other);
    /** Multiplies by a nonnegative machine factor. */
    void multiply(std::uint32_t factor);
    /** Computes a signed floor quotient that fits int32_t. */
    std::int32_t quotient(big_integer const& divisor) const;
};

/** Extracts one tentative digit from a linear fractional transform. */
std::int32_t extract_digit(big_integer const& q, big_integer const& r, big_integer const& t, std::uint32_t position);
#endif
