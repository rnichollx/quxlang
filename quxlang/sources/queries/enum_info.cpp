// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_int.hpp>
#include <quxlang/queries/specs/enum_info_spec.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <set>

rpnx::querygraph::coroutine< quxlang::enum_info_spec > quxlang::enum_info_impl(type_symbol input)
{
    ast2_symboid symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_enum_declaration >(symboid))
    {
        throw compiler_bug("enum_info requested for non-enum type: " + to_string(input));
    }

    ast2_enum_declaration const& declaration = as< ast2_enum_declaration >(symboid);
    type_symbol const evaluation_context = type_parent(input).value_or(type_symbol(context_reference{}));

    auto evaluate_u64 = [&](expression const& expr) -> rpnx::querygraph::coroutine< enum_info_spec >::cosubroutine< std::uint64_t >
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

    auto required_bits = [](std::uint64_t value) -> std::uint64_t
    {
        std::uint64_t bits = 1;
        while (bits < 64 && (value >> bits) != 0)
        {
            ++bits;
        }
        return bits;
    };

    auto value_fits_bits = [](std::uint64_t value, std::uint64_t bits) -> bool
    {
        if (bits >= 64)
        {
            return true;
        }
        return (value >> bits) == 0;
    };

    struct pending_enum_value
    {
        std::string name;
        bool is_null = false;
        bool is_default = false;
        bool is_explicit = false;
        std::optional< std::uint64_t > value;
    };

    enum_info result;
    result.allow_unknown = declaration.allow_unknown;
    result.is_ipc = declaration.is_ipc;
    std::vector< pending_enum_value > pending_values;
    std::set< std::string > names;
    std::set< std::uint64_t > used_values;

    auto value_is_reserved = [&](std::uint64_t value) -> bool
    {
        for (enum_reserved_range_info const& reserved : result.reserved_ranges)
        {
            if (value >= reserved.from && value <= reserved.to)
            {
                return true;
            }
        }
        return false;
    };

    for (ast2_enum_entry const& entry : declaration.entries)
    {
        if (typeis< ast2_enum_reserved_range_declaration >(entry))
        {
            ast2_enum_reserved_range_declaration const& reserved_decl = as< ast2_enum_reserved_range_declaration >(entry);
            std::uint64_t from = co_await evaluate_u64(reserved_decl.from);
            std::uint64_t to = co_await evaluate_u64(reserved_decl.to);
            if (from > to)
            {
                throw semantic_compilation_error("ENUM RESERVED range has FROM greater than TO: " + to_string(input));
            }
            result.reserved_ranges.push_back(enum_reserved_range_info{.from = from, .to = to});
            continue;
        }

        ast2_enum_value_declaration const& value_decl = as< ast2_enum_value_declaration >(entry);
        if (!names.insert(value_decl.name).second)
        {
            throw semantic_compilation_error("Duplicate ENUM value name '" + value_decl.name + "' in " + to_string(input));
        }

        pending_enum_value value;
        value.name = value_decl.name;
        value.is_null = value_decl.is_null;
        value.is_default = value_decl.is_default;
        if (value_decl.is_null && value_decl.value.has_value())
        {
            throw compiler_bug("enum parser produced both NULL and numeric value");
        }
        if (value_decl.is_null)
        {
            value.value = 0;
            value.is_explicit = true;
        }
        else if (value_decl.value.has_value())
        {
            value.value = co_await evaluate_u64(*value_decl.value);
            value.is_explicit = true;
        }
        pending_values.push_back(std::move(value));
    }

    bool saw_null = false;
    for (pending_enum_value const& value : pending_values)
    {
        if (!value.value.has_value())
        {
            continue;
        }
        if (value_is_reserved(*value.value))
        {
            throw semantic_compilation_error("ENUM value '" + value.name + "' overlaps a RESERVED range in " + to_string(input));
        }
        if (!used_values.insert(*value.value).second)
        {
            throw semantic_compilation_error("Duplicate ENUM numeric value in " + to_string(input));
        }
        if (value.is_null)
        {
            if (saw_null)
            {
                throw semantic_compilation_error("ENUM declares more than one NULL value: " + to_string(input));
            }
            saw_null = true;
        }
    }

    for (pending_enum_value& value : pending_values)
    {
        if (value.value.has_value())
        {
            continue;
        }
        std::uint64_t candidate = 0;
        while (used_values.contains(candidate) || value_is_reserved(candidate))
        {
            if (candidate == std::numeric_limits< std::uint64_t >::max())
            {
                throw semantic_compilation_error("ENUM implicit value allocation overflow in " + to_string(input));
            }
            ++candidate;
        }
        value.value = candidate;
        used_values.insert(candidate);
    }

    std::uint64_t max_value = 0;
    for (enum_reserved_range_info const& reserved : result.reserved_ranges)
    {
        max_value = std::max(max_value, reserved.to);
    }

    for (pending_enum_value const& pending : pending_values)
    {
        if (!pending.value.has_value())
        {
            throw compiler_bug("ENUM value remained unassigned");
        }

        enum_value_info value;
        value.name = pending.name;
        value.value = *pending.value;
        value.is_null = pending.is_null;
        value.is_default = pending.is_default || pending.is_null;
        value.is_explicit = pending.is_explicit;
        result.values.push_back(value);
        max_value = std::max(max_value, value.value);

        if (value.is_null)
        {
            result.null_value_name = value.name;
        }
        if (value.is_default)
        {
            if (result.default_value_name.has_value() && *result.default_value_name != value.name)
            {
                throw semantic_compilation_error("ENUM declares more than one DEFAULT value: " + to_string(input));
            }
            result.default_value_name = value.name;
        }
    }

    if (declaration.bit_width.has_value())
    {
        result.bits = co_await evaluate_u64(*declaration.bit_width);
        if (result.bits == 0 || result.bits > 64)
        {
            throw semantic_compilation_error("ENUM BITS must be between 1 and 64 for " + to_string(input));
        }
    }
    else
    {
        result.bits = required_bits(max_value);
    }

    for (enum_reserved_range_info const& reserved : result.reserved_ranges)
    {
        if (!value_fits_bits(reserved.to, result.bits))
        {
            throw semantic_compilation_error("ENUM RESERVED range does not fit BITS width in " + to_string(input));
        }
    }
    for (enum_value_info const& value : result.values)
    {
        if (!value_fits_bits(value.value, result.bits))
        {
            throw semantic_compilation_error("ENUM value '" + value.name + "' does not fit BITS width in " + to_string(input));
        }
    }

    result.storage_bytes = (result.bits + 7) / 8;
    co_return result;
}
