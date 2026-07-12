// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUSION_LAYOUT_HEADER_GUARD
#define QUXLANG_DATA_FUSION_LAYOUT_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/class_placement_info.hpp>

#include <cstdint>
#include <optional>

#include <rpnx/macros.hpp>

namespace quxlang
{
    /// Describes the target-specific physical placement of a fusion object.
    struct fusion_layout
    {
        bool is_inline = false;
        class_placement_info placement{.size = 0, .alignment = 1};
        class_placement_info payload_placement{.size = 0, .alignment = 1};
        std::uint64_t payload_offset = 0;
        std::uint64_t tag_offset = 0;
        type_symbol tag_type;
        std::optional< std::uint64_t > valueless_tag;

        RPNX_MEMBER_METADATA(fusion_layout, is_inline, placement, payload_placement, payload_offset, tag_offset, tag_type, valueless_tag);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_FUSION_LAYOUT_HEADER_GUARD
