// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/fixed_bytemath.hpp>
#include <quxlang/manipulators/numeric_literal_utils.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/specs/enum_info_spec.hpp>

#include "query_helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>

namespace quxlang::enum_info_detail
{
    /// Returns the number of bits required to encode a nonnegative integer.
    auto required_unsigned_bits(quxlang::bytemath::sle_int_unlimited value) -> std::uint64_t
    {
        value = quxlang::bytemath::normalize_signed(std::move(value));
        if (value.is_negative)
        {
            throw quxlang::compiler_bug("required_unsigned_bits received a negative value");
        }

        quxlang::bytemath::detail::le_trim_raw(value.data);
        if (value.data.empty() || (value.data.size() == 1 && value.data.front() == std::byte{0}))
        {
            return 1;
        }

        std::uint8_t high_byte = std::to_integer< std::uint8_t >(value.data.back());
        std::uint64_t result = static_cast< std::uint64_t >(value.data.size() - 1) * 8;
        while (high_byte != 0)
        {
            ++result;
            high_byte >>= 1;
        }
        return result;
    }

    /// Encodes a semantic integer in the exact canonical representation required by an ENUM.
    auto encode_integer(quxlang::bytemath::sle_int_unlimited value, quxlang::enum_integer_format const& format) -> std::vector< std::byte >
    {
        if (format.bit_width == 0 || format.bit_width > std::numeric_limits< std::size_t >::max())
        {
            throw quxlang::semantic_compilation_error("ENUM bit width cannot be represented by this compiler");
        }

        quxlang::bytemath::fixed_int_options options;
        options.bits = static_cast< std::size_t >(format.bit_width);
        options.has_sign = format.encoding == quxlang::enum_integer_encoding::signed_twos_complement_le;
        options.overflow_undefined = true;
        quxlang::bytemath::int_result encoded = quxlang::bytemath::unlimited_to_fixed(options, std::move(value));
        if (encoded.result_is_undefined)
        {
            throw quxlang::semantic_compilation_error("ENUM value does not fit its declared representation");
        }
        return std::move(encoded.data_bytes);
    }
}

rpnx::querygraph::coroutine< quxlang::enum_info_spec > quxlang::enum_info_impl(type_symbol input)
{
    if (typeis< builtin_symbol >(input) && is_builtin_enum_name(as< builtin_symbol >(input).name))
    {
        enum_info result;
        result.format.bit_width = 8;
        result.format.encoding = enum_integer_encoding::signed_twos_complement_le;
        result.values.emplace("LESS", enum_value_info{.value = {std::byte{0xff}}, .is_explicit = true});
        result.values.emplace("EQUAL", enum_value_info{.value = {std::byte{0x00}}, .is_default = true, .is_explicit = true});
        result.values.emplace("GREATER", enum_value_info{.value = {std::byte{0x01}}, .is_explicit = true});
        result.default_value_name = "EQUAL";
        co_return result;
    }

    ast2_symboid symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_enum_declaration >(symboid))
    {
        throw compiler_bug("enum_info requested for non-enum type: " + to_string(input));
    }

    ast2_enum_declaration const& declaration = as< ast2_enum_declaration >(symboid);
    type_symbol const evaluation_context = type_parent(input).value_or(type_symbol(context_reference{}));

    auto evaluate_integer = [&](expression const& expr) -> rpnx::querygraph::coroutine< enum_info_spec >::cosubroutine< bytemath::sle_int_unlimited >
    {
        if (typeis< expression_numeric_literal >(expr))
        {
            co_return literal_to_sle(as< expression_numeric_literal >(expr).value);
        }
        if (typeis< expression_char_literal >(expr))
        {
            co_return bytemath::sle_int_unlimited(static_cast< std::uint64_t >(as< expression_char_literal >(expr).value));
        }

        constexpr_input_v3 eval_input;
        eval_input.expr = expr;
        eval_input.context = evaluation_context;
        constexpr_numeric numeric = co_await rpnx::querygraph::request< constexpr_eval_numeric_query >(std::move(eval_input));
        std::string decimal;
        decimal.reserve(numeric.bytes.size());
        for (std::byte byte : numeric.bytes)
        {
            decimal.push_back(static_cast< char >(std::to_integer< std::uint8_t >(byte)));
        }
        co_return literal_to_sle(decimal);
    };

    auto evaluate_width = [&](expression const& expr) -> rpnx::querygraph::coroutine< enum_info_spec >::cosubroutine< std::uint64_t >
    {
        if (typeis< expression_numeric_literal >(expr))
        {
            bytemath::sle_int_unlimited width = literal_to_sle(as< expression_numeric_literal >(expr).value);
            bytemath::fixed_int_options options{.has_sign = false, .overflow_undefined = true, .bits = 64};
            auto [result, valid] = bytemath::unlimited_to_int< std::uint64_t >(options, std::move(width));
            if (!valid)
            {
                throw semantic_compilation_error("ENUM BITS width is too large for this compiler");
            }
            co_return result;
        }
        co_return co_await rpnx::querygraph::request< constexpr_u64_query >(constexpr_input{.expr = expr, .context = evaluation_context});
    };

    enum_info result;
    result.allow_unknown = declaration.allow_unknown;
    result.is_ipc = declaration.is_ipc;
    result.format.encoding = enum_integer_encoding::unsigned_le;

    std::vector< detail::enum_info_pending_value > pending_values;
    std::vector< detail::enum_info_pending_range > pending_ranges;
    std::set< std::string > names;
    std::set< bytemath::sle_int_unlimited > used_values;

    auto require_unsigned = [&](bytemath::sle_int_unlimited value, std::string const& description) -> bytemath::sle_int_unlimited
    {
        value = bytemath::normalize_signed(std::move(value));
        if (value.is_negative)
        {
            throw semantic_compilation_error(description + " cannot be negative in " + to_string(input));
        }
        return value;
    };

    for (ast2_enum_entry const& entry : declaration.entries)
    {
        if (typeis< ast2_enum_reserved_range_declaration >(entry))
        {
            ast2_enum_reserved_range_declaration const& range = as< ast2_enum_reserved_range_declaration >(entry);
            bytemath::sle_int_unlimited from = require_unsigned(co_await evaluate_integer(range.from), "ENUM RESERVED lower bound");
            bytemath::sle_int_unlimited to = require_unsigned(co_await evaluate_integer(range.to), "ENUM RESERVED upper bound");
            if (to < from)
            {
                throw semantic_compilation_error("ENUM RESERVED range has FROM greater than TO: " + to_string(input));
            }
            pending_ranges.push_back(detail::enum_info_pending_range{.from = std::move(from), .to = std::move(to)});
            continue;
        }

        ast2_enum_value_declaration const& declaration_value = as< ast2_enum_value_declaration >(entry);
        if (!names.insert(declaration_value.name).second)
        {
            throw semantic_compilation_error("Duplicate ENUM value name '" + declaration_value.name + "' in " + to_string(input));
        }

        detail::enum_info_pending_value value;
        value.name = declaration_value.name;
        value.is_null = declaration_value.is_null;
        value.is_default = declaration_value.is_default;
        if (declaration_value.is_null && declaration_value.value.has_value())
        {
            throw compiler_bug("enum parser produced both NULL and numeric value");
        }
        if (declaration_value.is_null)
        {
            value.value = bytemath::sle_int_unlimited(0);
            value.is_explicit = true;
        }
        else if (declaration_value.value.has_value())
        {
            value.value = require_unsigned(co_await evaluate_integer(*declaration_value.value), "ENUM value");
            value.is_explicit = true;
        }
        pending_values.push_back(std::move(value));
    }

    auto value_is_reserved = [&](bytemath::sle_int_unlimited const& value) -> bool
    {
        return std::any_of(pending_ranges.begin(), pending_ranges.end(), [&](detail::enum_info_pending_range const& range)
        {
            return !(value < range.from) && !(range.to < value);
        });
    };

    bool saw_null = false;
    for (detail::enum_info_pending_value const& value : pending_values)
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
        if (value.is_null && saw_null)
        {
            throw semantic_compilation_error("ENUM declares more than one NULL value: " + to_string(input));
        }
        saw_null = saw_null || value.is_null;
    }

    bytemath::sle_int_unlimited candidate(0);
    for (detail::enum_info_pending_value& value : pending_values)
    {
        if (value.value.has_value())
        {
            continue;
        }
        while (true)
        {
            if (used_values.contains(candidate))
            {
                candidate = bytemath::unlimited_int_signed_add_le(std::move(candidate), bytemath::sle_int_unlimited(1));
                continue;
            }

            std::vector< detail::enum_info_pending_range >::const_iterator const reserved = std::find_if(pending_ranges.begin(), pending_ranges.end(), [&](detail::enum_info_pending_range const& range)
            {
                return !(candidate < range.from) && !(range.to < candidate);
            });
            if (reserved == pending_ranges.end())
            {
                break;
            }
            candidate = bytemath::unlimited_int_signed_add_le(reserved->to, bytemath::sle_int_unlimited(1));
        }
        value.value = candidate;
        used_values.insert(candidate);
    }

    std::uint64_t inferred_bits = 1;
    for (detail::enum_info_pending_range const& range : pending_ranges)
    {
        inferred_bits = std::max(inferred_bits, enum_info_detail::required_unsigned_bits(range.to));
    }
    for (detail::enum_info_pending_value const& value : pending_values)
    {
        inferred_bits = std::max(inferred_bits, enum_info_detail::required_unsigned_bits(*value.value));
    }

    result.format.bit_width = declaration.bit_width.has_value() ? co_await evaluate_width(*declaration.bit_width) : inferred_bits;
    if (result.format.bit_width == 0)
    {
        throw semantic_compilation_error("ENUM BITS must be greater than zero for " + to_string(input));
    }

    for (detail::enum_info_pending_range const& range : pending_ranges)
    {
        result.reserved_ranges.push_back(enum_reserved_range_info{
            .from = enum_info_detail::encode_integer(range.from, result.format),
            .to = enum_info_detail::encode_integer(range.to, result.format),
        });
    }

    for (detail::enum_info_pending_value const& pending : pending_values)
    {
        enum_value_info value;
        value.value = enum_info_detail::encode_integer(*pending.value, result.format);
        value.is_null = pending.is_null;
        value.is_default = pending.is_default || pending.is_null;
        value.is_explicit = pending.is_explicit;

        if (value.is_null)
        {
            result.null_value_name = pending.name;
        }
        if (value.is_default)
        {
            if (result.default_value_name.has_value() && *result.default_value_name != pending.name)
            {
                throw semantic_compilation_error("ENUM declares more than one DEFAULT value: " + to_string(input));
            }
            result.default_value_name = pending.name;
        }
        result.values.emplace(pending.name, std::move(value));
    }

    co_return result;
}
