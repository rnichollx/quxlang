// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_VMIR2_HEADER_GUARD
#define QUXLANG_PARSERS_VMIR2_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <optional>
#include <utility>
#include <quxlang/parsers/integer.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/string_literal.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang::parsers::vmir2
{

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

    inline vm_invocation_args parse_invocation_args(parsing_context& ctx)
    {
        auto& ipos = ctx.iter_pos;
        auto end = ctx.iter_end;
        vm_invocation_args result;

        consume_symbol(ipos, end, '[');
        skip_whitespace(ipos, end);

    named_arg:
        skip_whitespace(ipos, end);
        if (auto str = try_parse_string_literal(ipos, end); str.has_value())
        {
            skip_whitespace(ipos, end);
            consume_symbol(ipos, end, '=');
            skip_whitespace(ipos, end);
            result.named[std::move(*str)] = parse_vmir_register(ipos, end);
            skip_whitespace(ipos, end);
            if (skip_symbol_if_is(ipos, end, ','))
                goto named_arg;
            else if (skip_symbol_if_is(ipos, end, ']'))
                return result;
            else
                throw syntax_compilation_error("Expected ',' or ']' after named argument");
        }
    positional_arg:
        skip_whitespace(ipos, end);
        if (skip_symbol_if_is(ipos, end, ']'))
            return result;
        result.positional.push_back(parse_vmir_register(ipos, end));
        skip_whitespace(ipos, end);
        if (skip_symbol_if_is(ipos, end, ','))
            goto positional_arg;
        else if (skip_symbol_if_is(ipos, end, ']'))
            return result;
        else
            throw syntax_compilation_error("Expected ',' or ']' after positional argument");
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
        result.base_index = parse_vmir_register(ipos, end);
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ',');
        skip_whitespace(ipos, end);
        result.store_index = parse_vmir_register(ipos, end);
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ',');
        skip_whitespace(ipos, end);
        result.offset = parse_uinteger(ipos, end);

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
        consume_symbol(ipos, end, ',');
        skip_whitespace(ipos, end);
        result.target = parse_vmir_register(ipos, end);

        return result;
    }

    inline std::optional< quxlang::vmir2::vm_instruction > try_parse_instruction(parsing_context& ctx)
    {
        std::optional< quxlang::vmir2::vm_instruction > result;
        result = try_parse_access_field(ctx);
        if (result.has_value())
        {
            return std::move(result);
        }
        result = try_parse_invoke(ctx);
        if (result.has_value())
        {
            return std::move(result);
        }
        return std::nullopt;
    }

} // namespace quxlang::parsers::vmir2

#endif // RPNX_QUXLANG_VMIR2_HEADER
