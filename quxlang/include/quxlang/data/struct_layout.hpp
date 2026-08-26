// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_STRUCT_LAYOUT_HEADER_GUARD
#define QUXLANG_DATA_STRUCT_LAYOUT_HEADER_GUARD

#include "struct_field_info.hpp"
#include <quxlang/data/struct_inheritance.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace quxlang
{
    /** Physical placement of one direct base within a complete struct layout. */
    struct struct_base_layout_info
    {
        struct_subobject_id subobject;
        std::optional< std::string > selector_name;
        type_symbol type;
        inheritance_kind kind = inheritance_kind::nonvirtual;
        std::size_t declaration_ordinal = 0;
        std::int64_t offset = 0;

        RPNX_MEMBER_METADATA(struct_base_layout_info, subobject, selector_name, type, kind, declaration_ordinal, offset);
    };

    /** Physical placement of one canonical virtual base within a complete object. */
    struct struct_virtual_base_layout_info
    {
        struct_subobject_id subobject;
        type_symbol type;
        std::size_t virtual_ordinal = 0;
        std::int64_t offset = 0;

        RPNX_MEMBER_METADATA(struct_virtual_base_layout_info, subobject, type, virtual_ordinal, offset);
    };

    /** Placement and ownership of the complete type's root runtime header. */
    struct struct_runtime_header_layout
    {
        std::uint64_t offset = 0;
        struct_polymorphism_kind polymorphism = struct_polymorphism_kind::none;
        std::optional< struct_subobject_id > primary_base;

        RPNX_MEMBER_METADATA(struct_runtime_header_layout, offset, polymorphism, primary_base);
    };

    /** Describes semantic base, field, runtime-header, and complete-object placement. */
    struct struct_layout
    {
        std::vector< struct_base_layout_info > direct_bases;
        std::vector< struct_virtual_base_layout_info > virtual_bases;
        std::vector< struct_field_info > fields;
        std::optional< struct_runtime_header_layout > runtime_header;
        std::uint64_t nonvirtual_size = 0;
        std::uint64_t nonvirtual_align = 1;
        std::uint64_t complete_size = 0;
        std::uint64_t complete_align = 1;

        RPNX_MEMBER_METADATA(struct_layout, direct_bases, virtual_bases, fields, runtime_header, nonvirtual_size, nonvirtual_align, complete_size, complete_align);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_STRUCT_LAYOUT_HEADER_GUARD
