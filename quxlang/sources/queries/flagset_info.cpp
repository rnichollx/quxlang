// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/fixed_bytemath.hpp>
#include <quxlang/manipulators/numeric_literal_utils.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_int.hpp>
#include <quxlang/queries/specs/flagset_info_spec.hpp>

#include "query_helpers.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

rpnx::querygraph::coroutine< quxlang::flagset_info_spec > quxlang::flagset_info_impl(type_symbol input)
{
    ast2_symboid symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_flagset_declaration >(symboid))
    {
        throw compiler_bug("flagset_info requested for non-flagset type: " + to_string(input));
    }

    ast2_flagset_declaration const& declaration = as< ast2_flagset_declaration >(symboid);
    type_symbol const evaluation_context = type_parent(input).value_or(type_symbol(context_reference{}));

    auto evaluate_u64 = [&](expression const& expr) -> rpnx::querygraph::coroutine< flagset_info_spec >::cosubroutine< std::uint64_t >
    {
        if (typeis< expression_numeric_literal >(expr))
        {
            co_return parsers::str_to_int< std::uint64_t >(as< expression_numeric_literal >(expr).value);
        }
        if (typeis< expression_char_literal >(expr))
        {
            co_return static_cast< std::uint64_t >(as< expression_char_literal >(expr).value);
        }
        co_return co_await rpnx::querygraph::request< constexpr_u64_query >(constexpr_input{.expr = expr, .context = evaluation_context});
    };

    auto evaluate_mask = [&](expression const& expr) -> rpnx::querygraph::coroutine< flagset_info_spec >::cosubroutine< std::vector< std::byte > >
    {
        std::string decimal;
        if (typeis< expression_numeric_literal >(expr))
        {
            decimal = as< expression_numeric_literal >(expr).value;
        }
        else if (typeis< expression_char_literal >(expr))
        {
            decimal = std::to_string(static_cast< std::uint64_t >(as< expression_char_literal >(expr).value));
        }
        else
        {
            constexpr_result_v3 evaluated = co_await rpnx::querygraph::request< constexpr_eval_v3_query >(
                constexpr_input_v3{.expr = expr, .context = evaluation_context, .expected_result_type = auto_temploidic{}});
            constexpr_value const& result = evaluated.values.at(constexpr_primary_result_id);
            if (typeis< constexpr_numeric >(result))
            {
                for (std::byte byte : as< constexpr_numeric >(result).bytes)
                {
                    decimal.push_back(static_cast< char >(std::to_integer< std::uint8_t >(byte)));
                }
            }
            else
            {
                if (!evaluated.deduced_type.has_value() || !evaluated.deduced_type->type_is< int_type >())
                {
                    throw semantic_compilation_error("FLAGSET mask must be an integer in " + to_string(input));
                }
                int_type const& integer = evaluated.deduced_type->as< int_type >();
                antestatal_value const& value = constexpr_value_as_antestatal(result);
                bytemath::sle_int_unlimited mask = bytemath::le_int_fixed_to_unlimited(
                    bytemath::fixed_int_options{.has_sign = integer.has_sign, .bits = integer.bits},
                    as< antestatal_primitive >(value).value);
                if (mask.is_negative)
                {
                    throw semantic_compilation_error("FLAGSET masks must be nonnegative in " + to_string(input));
                }
                bytemath::detail::le_trim_raw(mask.data);
                co_return std::move(mask.data);
            }
        }
        bytemath::sle_int_unlimited value = bytemath::normalize_signed(literal_to_sle(decimal));
        if (value.is_negative)
        {
            throw semantic_compilation_error("FLAGSET masks must be nonnegative in " + to_string(input));
        }
        bytemath::detail::le_trim_raw(value.data);
        co_return std::move(value.data);
    };

    auto merge_mask = [](std::vector< std::byte >& destination, std::vector< std::byte > const& mask)
    {
        destination.resize(std::max(destination.size(), mask.size()));
        for (std::size_t i = 0; i < mask.size(); ++i)
        {
            destination[i] |= mask[i];
        }
    };

    auto required_bits_for_mask = [](std::vector< std::byte > const& mask) -> std::uint64_t
    {
        for (std::size_t i = mask.size(); i > 0; --i)
        {
            if (mask[i - 1] != std::byte{0})
            {
                return (i - 1) * 8 + std::bit_width(std::to_integer< unsigned int >(mask[i - 1]));
            }
        }
        return 1;
    };

    flagset_info result;
    std::vector< detail::flagset_info_pending_value > pending_values;
    std::set< std::string > names;
    std::vector< std::byte > occupied_bits;

    for (ast2_flagset_entry const& entry : declaration.entries)
    {
        if (typeis< ast2_flagset_reserved_declaration >(entry))
        {
            ast2_flagset_reserved_declaration const& reserved_decl = as< ast2_flagset_reserved_declaration >(entry);
            std::vector< std::byte > mask = co_await evaluate_mask(reserved_decl.mask);
            result.reserved_masks.push_back(flagset_reserved_mask_info{.mask = mask});
            merge_mask(result.reserved_bit_mask, mask);
            merge_mask(occupied_bits, mask);
            continue;
        }

        ast2_flagset_value_declaration const& value_decl = as< ast2_flagset_value_declaration >(entry);
        if (!names.insert(value_decl.name).second)
        {
            throw semantic_compilation_error("Duplicate FLAGSET value name '" + value_decl.name + "' in " + to_string(input));
        }

        detail::flagset_info_pending_value value;
        value.name = value_decl.name;
        if (value_decl.mask.has_value())
        {
            value.mask = co_await evaluate_mask(*value_decl.mask);
            value.is_explicit = true;
        }
        pending_values.push_back(std::move(value));
    }

    for (detail::flagset_info_pending_value const& value : pending_values)
    {
        if (!value.mask.has_value())
        {
            continue;
        }
        if (std::ranges::all_of(*value.mask, [](std::byte byte) { return byte == std::byte{0}; }))
        {
            throw semantic_compilation_error("FLAGSET canonical value '" + value.name + "' cannot have a zero mask in " + to_string(input));
        }
        for (std::size_t i = 0; i < std::min(value.mask->size(), occupied_bits.size()); ++i)
        {
            if (((*value.mask)[i] & occupied_bits[i]) != std::byte{0})
            {
                throw semantic_compilation_error("FLAGSET canonical value '" + value.name + "' overlaps another canonical or RESERVED mask in " + to_string(input));
            }
        }
        merge_mask(occupied_bits, *value.mask);
        merge_mask(result.canonical_bit_mask, *value.mask);
    }

    std::optional< std::uint64_t > declared_bits;
    if (declaration.bit_width.has_value())
    {
        declared_bits = co_await evaluate_u64(*declaration.bit_width);
        if (*declared_bits == 0 || *declared_bits > std::numeric_limits< std::size_t >::max() - 7)
        {
            throw semantic_compilation_error("FLAGSET BITS must be positive and representable by this compiler for " + to_string(input));
        }
    }

    for (detail::flagset_info_pending_value& value : pending_values)
    {
        if (value.mask.has_value())
        {
            continue;
        }
        std::uint64_t bit_index = 0;
        while (bit_index / 8 < occupied_bits.size() && (occupied_bits[bit_index / 8] & std::byte{static_cast< unsigned char >(1U << (bit_index % 8))}) != std::byte{0})
        {
            ++bit_index;
        }
        if (declared_bits.has_value() && bit_index >= *declared_bits)
        {
            throw semantic_compilation_error("FLAGSET implicit value allocation overflow in " + to_string(input));
        }
        value.mask = std::vector< std::byte >(bit_index / 8 + 1);
        value.mask->back() = std::byte{static_cast< unsigned char >(1U << (bit_index % 8))};
        merge_mask(occupied_bits, *value.mask);
        merge_mask(result.canonical_bit_mask, *value.mask);
    }

    for (detail::flagset_info_pending_value const& pending : pending_values)
    {
        if (!pending.mask.has_value())
        {
            throw compiler_bug("FLAGSET value remained unassigned");
        }
        result.values.push_back(flagset_value_info{.name = pending.name, .mask = *pending.mask, .is_explicit = pending.is_explicit});
    }

    result.bits = declared_bits.value_or(required_bits_for_mask(occupied_bits));
    if (result.bits == 0 || result.bits > std::numeric_limits< std::size_t >::max() - 7)
    {
        throw semantic_compilation_error("FLAGSET BITS must be positive and representable by this compiler for " + to_string(input));
    }
    if (required_bits_for_mask(occupied_bits) > result.bits)
    {
        throw semantic_compilation_error("FLAGSET masks do not fit BITS width in " + to_string(input));
    }
    result.storage_bytes = (result.bits + 7) / 8;
    for (flagset_value_info& value : result.values)
    {
        value.mask.resize(result.storage_bytes);
    }
    for (flagset_reserved_mask_info& reserved : result.reserved_masks)
    {
        reserved.mask.resize(result.storage_bytes);
    }
    result.reserved_bit_mask.resize(result.storage_bytes);
    result.canonical_bit_mask.resize(result.storage_bytes);
    co_return result;
}
