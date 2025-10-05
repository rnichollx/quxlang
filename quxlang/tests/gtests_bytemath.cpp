// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/bytemath.hpp" // Include the header file with the functions to test
#include "gtest/gtest.h"
#include <stdexcept> // For std::invalid_argument
#include <string>
#include <vector>

// Use the namespace for convenience
using namespace quxlang;
using namespace quxlang::bytemath;
using namespace quxlang::bytemath::detail;

// Helper function to simplify testing comparison results
// Converts byte vectors to strings before comparison for readability
std::string compare_and_format(const std::vector< std::byte >& a, const std::vector< std::byte >& b)
{
    bool less = bytemath::detail::le_comp_less_raw(a, b);
    bool greater = bytemath::detail::le_comp_less_raw(b, a);
    bool equal = !less && !greater; // Or use le_comp_eq(a, b);

    std::string a_str = bytemath::detail::le_to_string_raw(a);
    std::string b_str = bytemath::detail::le_to_string_raw(b);

    if (less)
        return a_str + " < " + b_str;
    if (greater)
        return a_str + " > " + b_str;
    if (equal)
        return a_str + " == " + b_str;
    return "Comparison Error"; // Should not happen
}

// --- Test Cases ---

// Test le_unsigned_add
TEST(ByteMathAdd, BasicAddition)
{
    auto a = bytemath::detail::string_to_le_raw("123");
    auto b = bytemath::detail::string_to_le_raw("456");
    auto result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "579");
}

TEST(ByteMathAdd, AdditionWithCarry)
{
    auto a = string_to_le_raw("99");
    auto b = string_to_le_raw("1");
    auto result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "100");
}

TEST(ByteMathAdd, AdditionWithMultipleCarries)
{
    auto a = string_to_le_raw("999");
    auto b = string_to_le_raw("1");
    auto result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "1000");
}

TEST(ByteMathAdd, AdditionDifferentSizes)
{
    auto a = string_to_le_raw("1");
    auto b = string_to_le_raw("9999");
    auto result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "10000");

    a = string_to_le_raw("9999");
    b = string_to_le_raw("1");
    result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "10000");
}

TEST(ByteMathAdd, AddZero)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("0");
    auto result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12345");

    a = string_to_le_raw("0");
    b = string_to_le_raw("12345");
    result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12345");

    a = string_to_le_raw("0");
    b = string_to_le_raw("0");
    result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(ByteMathAdd, LargeNumbers)
{
    auto a = string_to_le_raw("12345678901234567890");
    auto b = string_to_le_raw("98765432109876543210");
    auto result = unlimited_int_unsigned_add_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "111111111011111111100");
}

// Test le_unsigned_sub (Assumes a >= b)
TEST(ByteMathSub, BasicSubtraction)
{
    auto a = string_to_le_raw("456");
    auto b = string_to_le_raw("123");
    auto result = unlimited_int_unsigned_sub_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "333");
}

TEST(ByteMathSub, SubtractionWithBorrow)
{
    auto a = string_to_le_raw("100");
    auto b = string_to_le_raw("1");
    auto result = unlimited_int_unsigned_sub_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "99");
}

TEST(ByteMathSub, SubtractionWithMultipleBorrows)
{
    auto a = string_to_le_raw("1000");
    auto b = string_to_le_raw("1");
    auto result = unlimited_int_unsigned_sub_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "999");
}

TEST(ByteMathSub, SubtractionDifferentSizes)
{
    auto a = string_to_le_raw("10000");
    auto b = string_to_le_raw("1");
    auto result = unlimited_int_unsigned_sub_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "9999");
}

TEST(ByteMathSub, SubtractZero)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("0");
    auto result = unlimited_int_unsigned_sub_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12345");
}

TEST(ByteMathSub, SubtractToZero)
{
    auto a = string_to_le_raw("123");
    auto b = string_to_le_raw("123");
    auto result = unlimited_int_unsigned_sub_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(ByteMathSub, LargeNumbers)
{
    auto a = string_to_le_raw("111111111011111111100");
    auto b = string_to_le_raw("98765432109876543210");
    auto result = unlimited_int_unsigned_sub_le_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12345678901234567890");
}

// Test le_unsigned_mult_raw
TEST(ByteMathMult, BasicMultiplication)
{
    auto a = string_to_le_raw("12");
    auto b = string_to_le_raw("10");
    auto result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "120");
}

TEST(ByteMathMult, MultiplyByZero)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("0");
    auto result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");

    a = string_to_le_raw("0");
    b = string_to_le_raw("12345");
    result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(ByteMathMult, MultiplyByOne)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("1");
    auto result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12345");

    a = string_to_le_raw("1");
    b = string_to_le_raw("12345");
    result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12345");
}

TEST(ByteMathMult, MultiplicationDifferentSizes)
{
    auto a = string_to_le_raw("100");
    auto b = string_to_le_raw("5");
    auto result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "500");

    a = string_to_le_raw("5");
    b = string_to_le_raw("100");
    result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "500");
}

TEST(ByteMathMult, LargeNumbers)
{
    auto a = string_to_le_raw("1000000000"); // 10^9
    auto b = string_to_le_raw("1000000000"); // 10^9
    auto result = le_unsigned_mult_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "1000000000000000000"); // 10^18
}

// Test le_unsigned_div_raw
TEST(ByteMathDiv, BasicDivision)
{
    auto a = string_to_le_raw("120");
    auto b = string_to_le_raw("10");
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12");
}

TEST(ByteMathDiv, DivisionWithRemainder)
{
    auto a = string_to_le_raw("123");
    auto b = string_to_le_raw("10");
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12"); // Integer division
}

TEST(ByteMathDiv, DivideByOne)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("1");
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "12345");
}

TEST(ByteMathDiv, DivideZero)
{
    auto a = string_to_le_raw("0");
    auto b = string_to_le_raw("123");
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(ByteMathDiv, DivideBySelf)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("12345");
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "1");
}

TEST(ByteMathDiv, DivideLargerBySmaller)
{
    auto a = string_to_le_raw("10");
    auto b = string_to_le_raw("123");
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(ByteMathDiv, DivisionByZero)
{
    // Division by zero should result in quotient 0 (as per implementation)
    auto a = string_to_le_raw("123");
    auto b = string_to_le_raw("0");
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(ByteMathDiv, LargeNumbers)
{
    auto a = string_to_le_raw("1000000000000000000"); // 10^18
    auto b = string_to_le_raw("1000000000");          // 10^9
    auto result = le_unsigned_div_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "1000000000"); // 10^9
}

// Test le_unsigned_mod_raw
TEST(bytemath, BasicModulo)
{
    auto a = string_to_le_raw("123");
    auto b = string_to_le_raw("10");
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "3");
}

TEST(bytemath, ModuloZeroRemainder)
{
    auto a = string_to_le_raw("120");
    auto b = string_to_le_raw("10");
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(bytemath, ModuloByOne)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("1");
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(bytemath, ModuloZero)
{
    auto a = string_to_le_raw("0");
    auto b = string_to_le_raw("123");
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(bytemath, ModuloBySelf)
{
    auto a = string_to_le_raw("12345");
    auto b = string_to_le_raw("12345");
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "0");
}

TEST(bytemath, ModuloLargerBySmaller)
{
    // a < b -> a % b == a
    auto a = string_to_le_raw("10");
    auto b = string_to_le_raw("123");
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "10");
}

TEST(bytemath, ModuloByZero)
{
    // Modulo by zero should return the original number 'a' (as per implementation)
    auto a = string_to_le_raw("123");
    auto b = string_to_le_raw("0");
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "123");
}

TEST(bytemath, LargeNumbers)
{
    auto a = string_to_le_raw("1000000000000000003"); // 10^18 + 3
    auto b = string_to_le_raw("1000000000");          // 10^9
    auto result = le_unsigned_mod_raw(a, b);
    EXPECT_EQ(le_to_string_raw(result), "3");
}

// Test le_comp_less and le_comp_eq
TEST(bytemath, LessThan)
{
    auto a = string_to_le_raw("123");
    auto b = string_to_le_raw("456");
    EXPECT_TRUE(detail::le_comp_less_raw(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(detail::le_comp_less_raw(b, a)) << compare_and_format(b, a);
}

TEST(bytemath, Equal)
{
    auto a = string_to_le_raw("123");
    auto b = string_to_le_raw("123");
    EXPECT_FALSE(detail::le_comp_less_raw(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(detail::le_comp_less_raw(b, a)) << compare_and_format(b, a);
    EXPECT_TRUE(le_comp_eq(a, b)) << compare_and_format(a, b); // Using le_comp_eq directly
}

TEST(bytemath, DifferentSizesLess)
{
    auto a = string_to_le_raw("99");
    auto b = string_to_le_raw("100");
    EXPECT_TRUE(detail::le_comp_less_raw(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(detail::le_comp_less_raw(b, a)) << compare_and_format(b, a);
}

TEST(bytemath, DifferentSizesGreater)
{
    auto a = string_to_le_raw("1000");
    auto b = string_to_le_raw("999");
    EXPECT_FALSE(detail::le_comp_less_raw(a, b)) << compare_and_format(a, b);
    EXPECT_TRUE(detail::le_comp_less_raw(b, a)) << compare_and_format(b, a);
}

TEST(bytemath, LeadingZerosInStringIgnored)
{
    // string_to_le_raw should handle this, but comparison should still work
    auto a = string_to_le_raw("00123");
    auto b = string_to_le_raw("123");
    EXPECT_TRUE(le_comp_eq(a, b)) << compare_and_format(a, b);
}

TEST(bytemath, ZeroComparison)
{
    auto a = string_to_le_raw("0");
    auto b = string_to_le_raw("1");
    auto c = string_to_le_raw("0");

    EXPECT_TRUE(detail::le_comp_less_raw(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(detail::le_comp_less_raw(b, a)) << compare_and_format(b, a);
    EXPECT_TRUE(le_comp_eq(a, c)) << compare_and_format(a, c);
}

// Test string_to_le_raw and le_to_string_raw conversions
TEST(ByteMathStringConv, RoundTrip)
{
    std::string original = "1234567890123456789012345";
    auto bytes = string_to_le_raw(original);
    std::string converted_back = le_to_string_raw(bytes);
    EXPECT_EQ(original, converted_back);
}

TEST(ByteMathStringConv, Zero)
{
    std::string original = "0";
    auto bytes = string_to_le_raw(original);
    std::string converted_back = le_to_string_raw(bytes);
    EXPECT_EQ(original, converted_back);
    // Check internal representation is {0x00} or empty (implementation dependent, le_to_string_raw handles both)
    // EXPECT_EQ(bytes.size(), 1); // Or 0 depending on trim/empty handling
    // if (!bytes.empty()) EXPECT_EQ(bytes[0], std::byte{0});
}

TEST(ByteMathStringConv, LeadingZeros)
{
    std::string original = "000123";
    auto bytes = string_to_le_raw(original);
    // string_to_le_raw should ignore leading zeros, le_to_string_raw shouldn't produce them (except for "0")
    std::string converted_back = le_to_string_raw(bytes);
    EXPECT_EQ(converted_back, "123");
}

TEST(ByteMathStringConv, InvalidInput)
{
    EXPECT_THROW(string_to_le_raw("123a45"), std::invalid_argument);
    EXPECT_THROW(string_to_le_raw(" 123"), std::invalid_argument);
    EXPECT_THROW(string_to_le_raw("-123"), std::invalid_argument);
}
