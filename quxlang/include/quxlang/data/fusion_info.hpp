// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUSION_INFO_HEADER_GUARD
#define QUXLANG_DATA_FUSION_INFO_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <rpnx/macros.hpp>

namespace quxlang
{
    /// Normalized declaration properties shared by UNION and VARIANT types.
    struct fusion_properties
    {
        bool is_inline = false;
        bool never_valueless = false;
        bool valueless_default = false;
        bool generate_copy = true;
        bool generate_move = true;
        bool generate_assignment = true;
        bool generate_swap = true;
        std::optional< std::uint64_t > default_index;

        RPNX_MEMBER_METADATA(fusion_properties, is_inline, never_valueless, valueless_default, generate_copy, generate_move, generate_assignment, generate_swap, default_index);
    };

    /// One normalized named alternative of a UNION type.
    struct union_option_info
    {
        std::string name;
        type_symbol type;

        RPNX_MEMBER_METADATA(union_option_info, name, type);
    };

    /// Normalized semantic information for a UNION type.
    struct union_info
    {
        std::vector< union_option_info > options;
        fusion_properties properties;

        RPNX_MEMBER_METADATA(union_info, options, properties);
    };

    /// Normalized semantic information for a VARIANT type.
    struct variant_info
    {
        std::vector< type_symbol > alternatives;
        fusion_properties properties;

        RPNX_MEMBER_METADATA(variant_info, alternatives, properties);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_FUSION_INFO_HEADER_GUARD
