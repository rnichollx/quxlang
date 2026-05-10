// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_ENUM_FLAGSET_INFO_HEADER_GUARD
#define QUXLANG_DATA_ENUM_FLAGSET_INFO_HEADER_GUARD

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <rpnx/macros.hpp>

namespace quxlang
{
    /// A normalized named value of an ENUM declaration.
    struct enum_value_info
    {
        std::string name;
        std::uint64_t value = 0;
        bool is_null = false;
        bool is_default = false;
        bool is_explicit = false;

        RPNX_MEMBER_METADATA(enum_value_info, name, value, is_null, is_default, is_explicit);
    };

    /// A normalized inclusive reserved storage-value range of an ENUM declaration.
    struct enum_reserved_range_info
    {
        std::uint64_t from = 0;
        std::uint64_t to = 0;

        RPNX_MEMBER_METADATA(enum_reserved_range_info, from, to);
    };

    /// Normalized semantic information for a nominal ENUM type.
    struct enum_info
    {
        std::uint64_t bits = 0;
        std::uint64_t storage_bytes = 0;
        std::vector< enum_value_info > values;
        std::vector< enum_reserved_range_info > reserved_ranges;
        std::optional< std::string > null_value_name;
        std::optional< std::string > default_value_name;
        bool allow_unknown = false;

        RPNX_MEMBER_METADATA(enum_info, bits, storage_bytes, values, reserved_ranges, null_value_name, default_value_name, allow_unknown);
    };

    /// A normalized named canonical mask of a FLAGSET declaration.
    struct flagset_value_info
    {
        std::string name;
        std::uint64_t mask = 0;
        bool is_explicit = false;

        RPNX_MEMBER_METADATA(flagset_value_info, name, mask, is_explicit);
    };

    /// A normalized reserved mask of a FLAGSET declaration.
    struct flagset_reserved_mask_info
    {
        std::uint64_t mask = 0;

        RPNX_MEMBER_METADATA(flagset_reserved_mask_info, mask);
    };

    /// Normalized semantic information for a nominal FLAGSET type.
    struct flagset_info
    {
        std::uint64_t bits = 0;
        std::uint64_t storage_bytes = 0;
        std::vector< flagset_value_info > values;
        std::vector< flagset_reserved_mask_info > reserved_masks;
        std::uint64_t reserved_bit_mask = 0;
        std::uint64_t canonical_bit_mask = 0;

        RPNX_MEMBER_METADATA(flagset_info, bits, storage_bytes, values, reserved_masks, reserved_bit_mask, canonical_bit_mask);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_ENUM_FLAGSET_INFO_HEADER_GUARD
