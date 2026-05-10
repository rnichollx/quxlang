// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_int.hpp>
#include <quxlang/queries/specs/flagset_info_spec.hpp>

#include <algorithm>
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

    auto required_bits_for_mask = [](std::uint64_t mask) -> std::uint64_t
    {
        if (mask == 0)
        {
            return 1;
        }
        std::uint64_t bits = 1;
        while (bits < 64 && (mask >> bits) != 0)
        {
            ++bits;
        }
        return bits;
    };

    auto mask_fits_bits = [](std::uint64_t mask, std::uint64_t bits) -> bool
    {
        if (bits >= 64)
        {
            return true;
        }
        return (mask >> bits) == 0;
    };

    struct pending_flagset_value
    {
        std::string name;
        bool is_explicit = false;
        std::optional< std::uint64_t > mask;
    };

    flagset_info result;
    std::vector< pending_flagset_value > pending_values;
    std::set< std::string > names;
    std::uint64_t occupied_bits = 0;

    for (ast2_flagset_entry const& entry : declaration.entries)
    {
        if (typeis< ast2_flagset_reserved_declaration >(entry))
        {
            ast2_flagset_reserved_declaration const& reserved_decl = as< ast2_flagset_reserved_declaration >(entry);
            std::uint64_t mask = co_await evaluate_u64(reserved_decl.mask);
            result.reserved_masks.push_back(flagset_reserved_mask_info{.mask = mask});
            result.reserved_bit_mask |= mask;
            occupied_bits |= mask;
            continue;
        }

        ast2_flagset_value_declaration const& value_decl = as< ast2_flagset_value_declaration >(entry);
        if (!names.insert(value_decl.name).second)
        {
            throw semantic_compilation_error("Duplicate FLAGSET value name '" + value_decl.name + "' in " + to_string(input));
        }

        pending_flagset_value value;
        value.name = value_decl.name;
        if (value_decl.mask.has_value())
        {
            value.mask = co_await evaluate_u64(*value_decl.mask);
            value.is_explicit = true;
        }
        pending_values.push_back(std::move(value));
    }

    for (pending_flagset_value const& value : pending_values)
    {
        if (!value.mask.has_value())
        {
            continue;
        }
        if (*value.mask == 0)
        {
            throw semantic_compilation_error("FLAGSET canonical value '" + value.name + "' cannot have a zero mask in " + to_string(input));
        }
        if ((*value.mask & occupied_bits) != 0)
        {
            throw semantic_compilation_error("FLAGSET canonical value '" + value.name + "' overlaps another canonical or RESERVED mask in " + to_string(input));
        }
        occupied_bits |= *value.mask;
        result.canonical_bit_mask |= *value.mask;
    }

    std::optional< std::uint64_t > declared_bits;
    if (declaration.bit_width.has_value())
    {
        declared_bits = co_await evaluate_u64(*declaration.bit_width);
        if (*declared_bits == 0 || *declared_bits > 64)
        {
            throw semantic_compilation_error("FLAGSET BITS must be between 1 and 64 for " + to_string(input));
        }
    }

    for (pending_flagset_value& value : pending_values)
    {
        if (value.mask.has_value())
        {
            continue;
        }
        std::uint64_t bit_index = 0;
        while (bit_index < 64 && (occupied_bits & (std::uint64_t{1} << bit_index)) != 0)
        {
            ++bit_index;
        }
        if (bit_index >= 64 || (declared_bits.has_value() && bit_index >= *declared_bits))
        {
            throw semantic_compilation_error("FLAGSET implicit value allocation overflow in " + to_string(input));
        }
        value.mask = std::uint64_t{1} << bit_index;
        occupied_bits |= *value.mask;
        result.canonical_bit_mask |= *value.mask;
    }

    for (pending_flagset_value const& pending : pending_values)
    {
        if (!pending.mask.has_value())
        {
            throw compiler_bug("FLAGSET value remained unassigned");
        }
        result.values.push_back(flagset_value_info{.name = pending.name, .mask = *pending.mask, .is_explicit = pending.is_explicit});
    }

    result.bits = declared_bits.value_or(required_bits_for_mask(occupied_bits));
    if (result.bits == 0 || result.bits > 64)
    {
        throw semantic_compilation_error("FLAGSET BITS must be between 1 and 64 for " + to_string(input));
    }
    if (!mask_fits_bits(occupied_bits, result.bits))
    {
        throw semantic_compilation_error("FLAGSET masks do not fit BITS width in " + to_string(input));
    }
    result.storage_bytes = (result.bits + 7) / 8;
    co_return result;
}
