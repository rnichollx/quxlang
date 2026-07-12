// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/struct_math.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/fusion_layout_spec.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace quxlang
{
    /// Returns the smallest supported unsigned integer width that holds the requested state count.
    auto inline_tag_bits(std::uint64_t state_count) -> std::uint64_t
    {
        if (state_count <= (std::uint64_t{1} << 8))
        {
            return 8;
        }
        if (state_count <= (std::uint64_t{1} << 16))
        {
            return 16;
        }
        if (state_count <= (std::uint64_t{1} << 32))
        {
            return 32;
        }
        return 64;
    }
} // namespace quxlang

rpnx::querygraph::coroutine< quxlang::fusion_layout_spec > quxlang::fusion_layout_impl(type_symbol input)
{
    class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(input);
    std::optional< union_info > normalized_union;
    std::optional< variant_info > normalized_variant;
    fusion_properties const* properties = nullptr;
    std::vector< type_symbol const* > alternatives;

    if (kind == class_kind::union_)
    {
        normalized_union = co_await rpnx::querygraph::request< union_info_query >(input);
        properties = &normalized_union->properties;
        alternatives.reserve(normalized_union->options.size());
        for (union_option_info const& option : normalized_union->options)
        {
            alternatives.push_back(&option.type);
        }
    }
    else if (kind == class_kind::variant)
    {
        normalized_variant = co_await rpnx::querygraph::request< variant_info_query >(input);
        properties = &normalized_variant->properties;
        alternatives.reserve(normalized_variant->alternatives.size());
        for (type_symbol const& alternative : normalized_variant->alternatives)
        {
            alternatives.push_back(&alternative);
        }
    }
    else
    {
        throw compiler_bug("fusion_layout requested for non-fusion type: " + to_string(input));
    }

    if (properties == nullptr || alternatives.empty())
    {
        throw compiler_bug("fusion_layout received invalid normalized fusion information for: " + to_string(input));
    }

    std::uint64_t const alternative_count = static_cast< std::uint64_t >(alternatives.size());
    fusion_layout result;
    result.is_inline = properties->is_inline;
    if (!properties->never_valueless)
    {
        result.valueless_tag = alternative_count;
    }

    machine_target_info const machine = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});
    std::uint64_t const pointer_size = machine.pointer_size_bytes();
    std::uint64_t const pointer_alignment = machine.pointer_align();

    if (!properties->is_inline)
    {
        result.payload_placement = class_placement_info{.size = pointer_size, .alignment = pointer_alignment};
        result.payload_offset = 0;
        result.tag_offset = pointer_size;
        result.tag_type = int_type{.bits = pointer_size * 8, .has_sign = false};
        result.placement = class_placement_info{.size = pointer_size * 2, .alignment = pointer_alignment};
        co_return result;
    }

    for (type_symbol const* alternative : alternatives)
    {
        if (typeis< void_type >(*alternative))
        {
            continue;
        }
        class_placement_info const placement = co_await rpnx::querygraph::request< class_placement_info_query >(*alternative);
        result.payload_placement.size = std::max(result.payload_placement.size, placement.size);
        result.payload_placement.alignment = std::max(result.payload_placement.alignment, placement.alignment);
    }

    std::uint64_t state_count = alternative_count;
    if (!properties->never_valueless)
    {
        if (state_count == std::numeric_limits< std::uint64_t >::max())
        {
            throw semantic_compilation_error("Fusion has too many alternatives to represent a valueless tag: " + to_string(input));
        }
        ++state_count;
    }

    std::uint64_t const tag_bits = inline_tag_bits(state_count);
    std::uint64_t const tag_size = tag_bits / 8;
    std::uint64_t const tag_alignment = machine.integer_alignment_for_bits(tag_bits);
    result.tag_type = int_type{.bits = tag_bits, .has_sign = false};
    result.payload_offset = 0;
    result.tag_offset = result.payload_placement.size;
    advance_to_alignment(result.tag_offset, tag_alignment);

    result.placement.alignment = std::max(result.payload_placement.alignment, tag_alignment);
    result.placement.size = result.tag_offset + tag_size;
    advance_to_alignment(result.placement.size, result.placement.alignment);
    co_return result;
}
