// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "gtest/gtest.h"
#include "quxlang/bytemath.hpp" // Include the header file with the functions to test
#include <string>
#include <vector>
#include <stdexcept> // For std::invalid_argument

// Use the namespace for convenience
using namespace quxlang;
using namespace quxlang::bytemath;

// Helper function to simplify testing comparison results
// Converts byte vectors to strings before comparison for readability
std::string compare_and_format(const std::vector<std::byte>& a, const std::vector<std::byte>& b) {
    bool less = bytemath::le_comp_less(a, b);
    bool greater = bytemath::le_comp_less(b, a);
    bool equal = !less && !greater; // Or use le_comp_eq(a, b);

    std::string a_str = bytemath::le_to_string(a);
    std::string b_str = bytemath::le_to_string(b);

    if (less) return a_str + " < " + b_str;
    if (greater) return a_str + " > " + b_str;
    if (equal) return a_str + " == " + b_str;
    return "Comparison Error"; // Should not happen
}


// --- Test Cases ---

// Test le_unsigned_add
TEST(ByteMathAdd, BasicAddition) {
    auto a = bytemath::string_to_le("123");
    auto b = bytemath::string_to_le("456");
    auto result = bytemath::unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "579");
}

TEST(ByteMathAdd, AdditionWithCarry) {
    auto a = string_to_le("99");
    auto b = string_to_le("1");
    auto result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "100");
}

TEST(ByteMathAdd, AdditionWithMultipleCarries) {
    auto a = string_to_le("999");
    auto b = string_to_le("1");
    auto result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "1000");
}

TEST(ByteMathAdd, AdditionDifferentSizes) {
    auto a = string_to_le("1");
    auto b = string_to_le("9999");
    auto result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "10000");

    a = string_to_le("9999");
    b = string_to_le("1");
    result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "10000");
}

TEST(ByteMathAdd, AddZero) {
    auto a = string_to_le("12345");
    auto b = string_to_le("0");
    auto result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "12345");

    a = string_to_le("0");
    b = string_to_le("12345");
    result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "12345");

    a = string_to_le("0");
    b = string_to_le("0");
    result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(ByteMathAdd, LargeNumbers) {
    auto a = string_to_le("12345678901234567890");
    auto b = string_to_le("98765432109876543210");
    auto result = unlimited_int_unsigned_add_le(a, b);
    EXPECT_EQ(le_to_string(result), "111111111011111111100");
}

// Test le_unsigned_sub (Assumes a >= b)
TEST(ByteMathSub, BasicSubtraction) {
    auto a = string_to_le("456");
    auto b = string_to_le("123");
    auto result = unlimited_int_unsigned_sub_le(a, b);
    EXPECT_EQ(le_to_string(result), "333");
}

TEST(ByteMathSub, SubtractionWithBorrow) {
    auto a = string_to_le("100");
    auto b = string_to_le("1");
    auto result = unlimited_int_unsigned_sub_le(a, b);
    EXPECT_EQ(le_to_string(result), "99");
}

TEST(ByteMathSub, SubtractionWithMultipleBorrows) {
    auto a = string_to_le("1000");
    auto b = string_to_le("1");
    auto result = unlimited_int_unsigned_sub_le(a, b);
    EXPECT_EQ(le_to_string(result), "999");
}

TEST(ByteMathSub, SubtractionDifferentSizes) {
    auto a = string_to_le("10000");
    auto b = string_to_le("1");
    auto result = unlimited_int_unsigned_sub_le(a, b);
    EXPECT_EQ(le_to_string(result), "9999");
}

TEST(ByteMathSub, SubtractZero) {
    auto a = string_to_le("12345");
    auto b = string_to_le("0");
    auto result = unlimited_int_unsigned_sub_le(a, b);
    EXPECT_EQ(le_to_string(result), "12345");
}

TEST(ByteMathSub, SubtractToZero) {
    auto a = string_to_le("123");
    auto b = string_to_le("123");
    auto result = unlimited_int_unsigned_sub_le(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(ByteMathSub, LargeNumbers) {
    auto a = string_to_le("111111111011111111100");
    auto b = string_to_le("98765432109876543210");
    auto result = unlimited_int_unsigned_sub_le(a, b);
    EXPECT_EQ(le_to_string(result), "12345678901234567890");
}

// Test le_unsigned_mult
TEST(ByteMathMult, BasicMultiplication) {
    auto a = string_to_le("12");
    auto b = string_to_le("10");
    auto result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "120");
}

TEST(ByteMathMult, MultiplyByZero) {
    auto a = string_to_le("12345");
    auto b = string_to_le("0");
    auto result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "0");

    a = string_to_le("0");
    b = string_to_le("12345");
    result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(ByteMathMult, MultiplyByOne) {
    auto a = string_to_le("12345");
    auto b = string_to_le("1");
    auto result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "12345");

    a = string_to_le("1");
    b = string_to_le("12345");
    result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "12345");
}

TEST(ByteMathMult, MultiplicationDifferentSizes) {
    auto a = string_to_le("100");
    auto b = string_to_le("5");
    auto result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "500");

    a = string_to_le("5");
    b = string_to_le("100");
    result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "500");
}

TEST(ByteMathMult, LargeNumbers) {
    auto a = string_to_le("1000000000"); // 10^9
    auto b = string_to_le("1000000000"); // 10^9
    auto result = le_unsigned_mult(a, b);
    EXPECT_EQ(le_to_string(result), "1000000000000000000"); // 10^18
}

// Test le_unsigned_div
TEST(ByteMathDiv, BasicDivision) {
    auto a = string_to_le("120");
    auto b = string_to_le("10");
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "12");
}

TEST(ByteMathDiv, DivisionWithRemainder) {
    auto a = string_to_le("123");
    auto b = string_to_le("10");
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "12"); // Integer division
}

TEST(ByteMathDiv, DivideByOne) {
    auto a = string_to_le("12345");
    auto b = string_to_le("1");
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "12345");
}

TEST(ByteMathDiv, DivideZero) {
    auto a = string_to_le("0");
    auto b = string_to_le("123");
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(ByteMathDiv, DivideBySelf) {
    auto a = string_to_le("12345");
    auto b = string_to_le("12345");
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "1");
}

TEST(ByteMathDiv, DivideLargerBySmaller) {
    auto a = string_to_le("10");
    auto b = string_to_le("123");
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(ByteMathDiv, DivisionByZero) {
    // Division by zero should result in quotient 0 (as per implementation)
    auto a = string_to_le("123");
    auto b = string_to_le("0");
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(ByteMathDiv, LargeNumbers) {
    auto a = string_to_le("1000000000000000000"); // 10^18
    auto b = string_to_le("1000000000");         // 10^9
    auto result = le_unsigned_div(a, b);
    EXPECT_EQ(le_to_string(result), "1000000000"); // 10^9
}

// Test le_unsigned_mod
TEST(bytemath, BasicModulo) {
    auto a = string_to_le("123");
    auto b = string_to_le("10");
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "3");
}

TEST(bytemath, ModuloZeroRemainder) {
    auto a = string_to_le("120");
    auto b = string_to_le("10");
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(bytemath, ModuloByOne) {
    auto a = string_to_le("12345");
    auto b = string_to_le("1");
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(bytemath, ModuloZero) {
    auto a = string_to_le("0");
    auto b = string_to_le("123");
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(bytemath, ModuloBySelf) {
    auto a = string_to_le("12345");
    auto b = string_to_le("12345");
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "0");
}

TEST(bytemath, ModuloLargerBySmaller) {
    // a < b -> a % b == a
    auto a = string_to_le("10");
    auto b = string_to_le("123");
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "10");
}

TEST(bytemath, ModuloByZero) {
    // Modulo by zero should return the original number 'a' (as per implementation)
    auto a = string_to_le("123");
    auto b = string_to_le("0");
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "123");
}

TEST(bytemath, LargeNumbers) {
    auto a = string_to_le("1000000000000000003"); // 10^18 + 3
    auto b = string_to_le("1000000000");         // 10^9
    auto result = le_unsigned_mod(a, b);
    EXPECT_EQ(le_to_string(result), "3");
}

// Test le_comp_less and le_comp_eq
TEST(bytemath, LessThan) {
    auto a = string_to_le("123");
    auto b = string_to_le("456");
    EXPECT_TRUE(le_comp_less(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(le_comp_less(b, a)) << compare_and_format(b, a);
}

TEST(bytemath, Equal) {
    auto a = string_to_le("123");
    auto b = string_to_le("123");
    EXPECT_FALSE(le_comp_less(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(le_comp_less(b, a)) << compare_and_format(b, a);
    EXPECT_TRUE(le_comp_eq(a, b)) << compare_and_format(a, b); // Using le_comp_eq directly
}

TEST(bytemath, DifferentSizesLess) {
    auto a = string_to_le("99");
    auto b = string_to_le("100");
    EXPECT_TRUE(le_comp_less(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(le_comp_less(b, a)) << compare_and_format(b, a);
}

TEST(bytemath, DifferentSizesGreater) {
    auto a = string_to_le("1000");
    auto b = string_to_le("999");
    EXPECT_FALSE(le_comp_less(a, b)) << compare_and_format(a, b);
    EXPECT_TRUE(le_comp_less(b, a)) << compare_and_format(b, a);
}

TEST(bytemath, LeadingZerosInStringIgnored) {
    // string_to_le should handle this, but comparison should still work
    auto a = string_to_le("00123");
    auto b = string_to_le("123");
    EXPECT_TRUE(le_comp_eq(a, b)) << compare_and_format(a, b);
}

TEST(bytemath, ZeroComparison) {
    auto a = string_to_le("0");
    auto b = string_to_le("1");
    auto c = string_to_le("0");

    EXPECT_TRUE(le_comp_less(a, b)) << compare_and_format(a, b);
    EXPECT_FALSE(le_comp_less(b, a)) << compare_and_format(b, a);
    EXPECT_TRUE(le_comp_eq(a, c)) << compare_and_format(a, c);
}

// Test string_to_le and le_to_string conversions
TEST(ByteMathStringConv, RoundTrip) {
    std::string original = "1234567890123456789012345";
    auto bytes = string_to_le(original);
    std::string converted_back = le_to_string(bytes);
    EXPECT_EQ(original, converted_back);
}

TEST(ByteMathStringConv, Zero) {
    std::string original = "0";
    auto bytes = string_to_le(original);
    std::string converted_back = le_to_string(bytes);
    EXPECT_EQ(original, converted_back);
    // Check internal representation is {0x00} or empty (implementation dependent, le_to_string handles both)
    // EXPECT_EQ(bytes.size(), 1); // Or 0 depending on trim/empty handling
    // if (!bytes.empty()) EXPECT_EQ(bytes[0], std::byte{0});
}

TEST(ByteMathStringConv, LeadingZeros) {
    std::string original = "000123";
    auto bytes = string_to_le(original);
    // string_to_le should ignore leading zeros, le_to_string shouldn't produce them (except for "0")
    std::string converted_back = le_to_string(bytes);
    EXPECT_EQ(converted_back, "123");
}

TEST(ByteMathStringConv, InvalidInput) {
    EXPECT_THROW(string_to_le("123a45"), std::invalid_argument);
    EXPECT_THROW(string_to_le(" 123"), std::invalid_argument);
    EXPECT_THROW(string_to_le("-123"), std::invalid_argument);
}

