// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_VMIR2_HEADER_GUARD
#define QUXLANG_PARSERS_VMIR2_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <optional>
#include <string_view>
#include <utility>
#include <quxlang/parsers/integer.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/string_literal.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang::parsers::vmir2
{
    /// Skips one VMIR opcode without accepting a longer opcode that merely shares a prefix.
    template < typename It >
    bool skip_opcode_if_is(It& ipos, It end, std::string_view opcode)
    {
        It trial = ipos;
        for (char const ch : opcode)
        {
            if (trial == end || *trial != ch)
            {
                return false;
            }
            ++trial;
        }
        if (trial != end && ((*trial >= 'A' && *trial <= 'Z') || (*trial >= '0' && *trial <= '9') || *trial == '_'))
        {
            return false;
        }
        ipos = trial;
        return true;
    }

    template < typename It >
    std::size_t parse_vmir_register(It& ipos, It end)
    {
        if (!skip_symbol_if_is(ipos, end, "%"))
            throw syntax_compilation_error("Expected register symbol");

        return parse_integer(ipos, end);
    }

    template < typename It >
    std::optional< std::size_t > try_parse_vmir_register(It& ipos, It end)
    {
        if (!skip_symbol_if_is(ipos, end, "%"))
            return std::nullopt;

        return parse_integer(ipos, end);
    }

    /// Parses a VMIR block label reference such as `!3`.
    template < typename It >
    std::size_t parse_vmir_block(It& ipos, It end)
    {
        if (!skip_symbol_if_is(ipos, end, "!"))
            throw syntax_compilation_error("Expected block symbol");

        return parse_integer(ipos, end);
    }

    /// Parses an unsigned VMIR immediate such as `#3`.
    template < typename It >
    std::uint64_t parse_vmir_ordinal(It& ipos, It end)
    {
        if (!skip_symbol_if_is(ipos, end, "#"))
        {
            throw syntax_compilation_error("Expected ordinal immediate");
        }
        return parse_integer(ipos, end);
    }

    /** Parses one of the two-register fusion query instructions. */
    template < typename Instruction >
    std::optional< Instruction > try_parse_fusion_query(parsing_context& ctx, std::string_view opcode)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, opcode))
        {
            ipos = begin;
            return std::nullopt;
        }
        skip_whitespace(ipos, end);
        Instruction result;
        result.subject = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.result = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        result.location = ctx.get_location_optional(begin, ipos);
        return result;
    }

    /** Parses one of the fusion query instructions that names an alternative ordinal. */
    template < typename Instruction >
    std::optional< Instruction > try_parse_fusion_ordinal_query(parsing_context& ctx, std::string_view opcode)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, opcode))
        {
            ipos = begin;
            return std::nullopt;
        }
        skip_whitespace(ipos, end);
        Instruction result;
        result.subject = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.alternative = parse_vmir_ordinal(ipos, end);
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.result = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        result.location = ctx.get_location_optional(begin, ipos);
        return result;
    }

    /** Parses `FUSION_SET_ACTIVE target, #alternative[, payload]`. */
    inline std::optional< quxlang::vmir2::fusion_set_active > try_parse_fusion_set_active(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, "FUSION_SET_ACTIVE"))
        {
            ipos = begin;
            return std::nullopt;
        }
        skip_whitespace(ipos, end);
        quxlang::vmir2::fusion_set_active result;
        result.target = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.alternative = parse_vmir_ordinal(ipos, end);
        skip_whitespace(ipos, end);
        if (skip_symbol_if_is(ipos, end, ","))
        {
            skip_whitespace(ipos, end);
            result.payload_storage = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        }
        result.location = ctx.get_location_optional(begin, ipos);
        return result;
    }

    /** Parses a one-register fusion mutation instruction. */
    template < typename Instruction >
    std::optional< Instruction > try_parse_fusion_target(parsing_context& ctx, std::string_view opcode)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, opcode))
        {
            ipos = begin;
            return std::nullopt;
        }
        skip_whitespace(ipos, end);
        Instruction result;
        result.target = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        result.location = ctx.get_location_optional(begin, ipos);
        return result;
    }

    /** Parses `FUSION_SWAP_BOXED_STATE lhs, rhs`. */
    inline std::optional< quxlang::vmir2::fusion_swap_boxed_state > try_parse_fusion_swap_boxed_state(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, "FUSION_SWAP_BOXED_STATE"))
        {
            ipos = begin;
            return std::nullopt;
        }
        skip_whitespace(ipos, end);
        quxlang::vmir2::fusion_swap_boxed_state result;
        result.a = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.b = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        result.location = ctx.get_location_optional(begin, ipos);
        return result;
    }

    /** Parses `TABLEBRANCH index, [targets...], default`. */
    inline std::optional< quxlang::vmir2::tablebranch > try_parse_tablebranch(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, "TABLEBRANCH"))
        {
            ipos = begin;
            return std::nullopt;
        }
        skip_whitespace(ipos, end);
        quxlang::vmir2::tablebranch result;
        result.index = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, "[");
        skip_whitespace(ipos, end);
        if (!skip_symbol_if_is(ipos, end, "]"))
        {
            while (true)
            {
                result.targets.push_back(quxlang::vmir2::block_index(parse_vmir_block(ipos, end)));
                skip_whitespace(ipos, end);
                if (skip_symbol_if_is(ipos, end, "]"))
                {
                    break;
                }
                consume_symbol(ipos, end, ",");
                skip_whitespace(ipos, end);
            }
        }
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.default_target = quxlang::vmir2::block_index(parse_vmir_block(ipos, end));
        result.location = ctx.get_location_optional(begin, ipos);
        return result;
    }

    inline quxlang::vmir2::invocation_args parse_invocation_args(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        quxlang::vmir2::invocation_args result;

        consume_symbol(ipos, end, "[");
        skip_whitespace(ipos, end);

    named_arg:
        skip_whitespace(ipos, end);
        if (auto str = try_parse_string_literal(ipos, end); str.has_value())
        {
            skip_whitespace(ipos, end);
            consume_symbol(ipos, end, "=");
            skip_whitespace(ipos, end);
            result.named[std::move(*str)] = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
            skip_whitespace(ipos, end);
            if (skip_symbol_if_is(ipos, end, ","))
                goto named_arg;
            else if (skip_symbol_if_is(ipos, end, "]"))
                return result;
            else
                throw syntax_compilation_error("Expected ',' or ']' after named argument");
        }
    positional_arg:
        skip_whitespace(ipos, end);
        if (skip_symbol_if_is(ipos, end, "]"))
            return result;
        result.positional.push_back(quxlang::vmir2::local_index(parse_vmir_register(ipos, end)));
        skip_whitespace(ipos, end);
        if (skip_symbol_if_is(ipos, end, ","))
            goto positional_arg;
        else if (skip_symbol_if_is(ipos, end, "]"))
            return result;
        else
            throw syntax_compilation_error("Expected ',' or ']' after positional argument");
    }

    /// Parses an `INITGUARD_COMPLETE` instruction when it appears at the current input position.
    inline std::optional< quxlang::vmir2::initguard_complete > try_parse_initguard_complete(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, "INITGUARD_COMPLETE"))
        {
            ipos = begin;
            return std::nullopt;
        }

        skip_whitespace(ipos, end);
        return quxlang::vmir2::initguard_complete{.lock = quxlang::vmir2::local_index(parse_vmir_register(ipos, end))};
    }

    /// Parses an `INITGUARD_ABORT` instruction when it appears at the current input position.
    inline std::optional< quxlang::vmir2::initguard_abort > try_parse_initguard_abort(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, "INITGUARD_ABORT"))
        {
            ipos = begin;
            return std::nullopt;
        }

        skip_whitespace(ipos, end);
        return quxlang::vmir2::initguard_abort{.lock = quxlang::vmir2::local_index(parse_vmir_register(ipos, end))};
    }

    /// Parses the VMIR storage access class used by initguard acquisition.
    inline quxlang::vmir2::access_class parse_access_class(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        if (skip_keyword_if_is(ipos, end, "GLOBAL"))
        {
            return quxlang::vmir2::access_class::global;
        }
        if (skip_keyword_if_is(ipos, end, "THREAD"))
        {
            return quxlang::vmir2::access_class::thread;
        }
        throw syntax_compilation_error("Expected access class");
    }

    /// Parses an `INITGUARD_TRY_ACQUIRE` terminator when it appears at the current input position.
    inline std::optional< quxlang::vmir2::initguard_try_acquire > try_parse_initguard_try_acquire(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, "INITGUARD_TRY_ACQUIRE"))
        {
            ipos = begin;
            return std::nullopt;
        }

        skip_whitespace(ipos, end);
        quxlang::vmir2::initguard_try_acquire result;
        result.class_ = parse_access_class(ctx);
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.symbol = parse_type_symbol(ctx);
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.target_lock = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.target_acquired = quxlang::vmir2::block_index(parse_vmir_block(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.target_already_initialized = quxlang::vmir2::block_index(parse_vmir_block(ipos, end));
        return result;
    }

    /** Parses a `PANIC "message"` terminator when it appears at the current input position. */
    inline std::optional< quxlang::vmir2::panic > try_parse_panic(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_opcode_if_is(ipos, end, "PANIC"))
        {
            ipos = begin;
            return std::nullopt;
        }

        skip_whitespace(ipos, end);
        std::optional< std::string > message = try_parse_string_literal(ipos, end);
        if (!message.has_value())
        {
            throw syntax_compilation_error("Expected string literal after PANIC terminator");
        }
        quxlang::vmir2::panic result{.message = std::move(*message)};
        result.location = ctx.get_location_optional(begin, ipos);
        return result;
    }

    inline std::optional< quxlang::vmir2::access_field > try_parse_access_field(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_keyword_if_is(ipos, end, "ACF"))
        {
            ipos = begin;
            return std::nullopt;
        }

        skip_whitespace(ipos, end);

        quxlang::vmir2::access_field result;
        result.base_index = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.store_index = quxlang::vmir2::local_index(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        if (auto str = try_parse_string_literal(ipos, end); str.has_value())
        {
            result.field_name = std::move(*str);
        }
        else
        {
            result.field_name = parse_symbol(ipos, end);
        }

        return result;
    }

    inline std::optional< quxlang::vmir2::invoke > try_parse_invoke(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ipos;
        if (!skip_keyword_if_is(ipos, end, "IVK"))
        {
            ipos = begin;
            return std::nullopt;
        }

        skip_whitespace(ipos, end);

        quxlang::vmir2::invoke result;

        result.what = parse_type_symbol(ctx);
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ",");
        skip_whitespace(ipos, end);
        result.args = parse_invocation_args(ctx);

        return result;
    }

    inline std::optional< quxlang::vmir2::vm_instruction > try_parse_instruction(parsing_context& ctx)
    {
        std::optional< quxlang::vmir2::vm_instruction > result;
        if (auto instruction = try_parse_fusion_query< quxlang::vmir2::fusion_active_index >(ctx, "FUSION_ACTIVE_INDEX"); instruction.has_value()) return quxlang::vmir2::vm_instruction{*instruction};
        if (auto instruction = try_parse_fusion_ordinal_query< quxlang::vmir2::fusion_has_alternative >(ctx, "FUSION_HAS_ALTERNATIVE"); instruction.has_value()) return quxlang::vmir2::vm_instruction{*instruction};
        if (auto instruction = try_parse_fusion_query< quxlang::vmir2::fusion_is_valueless >(ctx, "FUSION_IS_VALUELESS"); instruction.has_value()) return quxlang::vmir2::vm_instruction{*instruction};
        if (auto instruction = try_parse_fusion_ordinal_query< quxlang::vmir2::fusion_storage_ref >(ctx, "FUSION_STORAGE_REF"); instruction.has_value()) return quxlang::vmir2::vm_instruction{*instruction};
        if (auto instruction = try_parse_fusion_set_active(ctx); instruction.has_value()) return quxlang::vmir2::vm_instruction{*instruction};
        if (auto instruction = try_parse_fusion_target< quxlang::vmir2::fusion_set_valueless >(ctx, "FUSION_SET_VALUELESS"); instruction.has_value()) return quxlang::vmir2::vm_instruction{*instruction};
        if (auto instruction = try_parse_fusion_swap_boxed_state(ctx); instruction.has_value()) return quxlang::vmir2::vm_instruction{*instruction};
        result = try_parse_access_field(ctx);
        if (result.has_value())
        {
            return result;
        }
        result = try_parse_invoke(ctx);
        if (result.has_value())
        {
            return result;
        }
        result = try_parse_initguard_complete(ctx);
        if (result.has_value())
        {
            return result;
        }
        result = try_parse_initguard_abort(ctx);
        if (result.has_value())
        {
            return result;
        }
        return std::nullopt;
    }

    inline std::optional< quxlang::vmir2::vm_terminator > try_parse_terminator(parsing_context& ctx)
    {
        if (std::optional< quxlang::vmir2::tablebranch > table = try_parse_tablebranch(ctx); table.has_value())
        {
            return quxlang::vmir2::vm_terminator{std::move(*table)};
        }
        if (std::optional< quxlang::vmir2::panic > panic = try_parse_panic(ctx); panic.has_value())
        {
            return quxlang::vmir2::vm_terminator{std::move(*panic)};
        }
        std::optional< quxlang::vmir2::initguard_try_acquire > result = try_parse_initguard_try_acquire(ctx);
        if (result.has_value())
        {
            return quxlang::vmir2::vm_terminator{*result};
        }
        return std::nullopt;
    }

} // namespace quxlang::parsers::vmir2

#endif // RPNX_QUXLANG_VMIR2_HEADER
