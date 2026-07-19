// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_CONSTEXPR_INTERPRETER_INTERNAL_HEADER_GUARD
#define QUXLANG_SOURCES_CONSTEXPR_INTERPRETER_INTERNAL_HEADER_GUARD

#include <quxlang/data/enum_flagset_info.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quxlang::constexpr_interpreter_detail
{
    /** Rejects enum metadata without a canonical fixed-width representation. */
    void require_canonical_enum_value(enum_info const& info, std::vector< std::byte > const& value);
    /** Returns the byte count required for a bit width. */
    auto bytes_for_bits(std::size_t bits) -> std::size_t;
    /** Applies a binary byte operation to two vectors. */
    template < typename Function >
    void bitwise_byte_op(std::vector< std::byte >& out, std::vector< std::byte > const& a, std::vector< std::byte > const& b, Function fn);
    /** Applies a binary byte operation to a mutable vector. */
    template < typename Function >
    void mut_bitwise_byte_op(std::vector< std::byte >& target, std::vector< std::byte > const& b, Function fn);
    /** Complements every byte in a vector. */
    void bitwise_not_inplace(std::vector< std::byte >& value);
    /** Truncates a little-endian byte vector to an exact bit width. */
    auto truncate_to_bits(std::vector< std::byte > data, std::size_t bits) -> std::vector< std::byte >;
    /** Decodes a little-endian unsigned integer. */
    auto bytes_to_u64(std::vector< std::byte > const& data) -> std::uint64_t;
    /** Returns the bitwise union of two byte vectors. */
    auto bit_or_vec(std::vector< std::byte > a, std::vector< std::byte > const& b) -> std::vector< std::byte >;
} // namespace quxlang::constexpr_interpreter_detail

#endif // QUXLANG_SOURCES_CONSTEXPR_INTERPRETER_INTERNAL_HEADER_GUARD
