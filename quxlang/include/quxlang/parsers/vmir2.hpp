//
// Created by Ryan Nicholl on 5/30/24.
//

#ifndef RPNX_QUXLANG_PARSERS_VMIR2_HEADER
#define RPNX_QUXLANG_PARSERS_VMIR2_HEADER

#include <optional>
#include <quxlang/parsers/integer.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/string_literal.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang::parsers::vmir2
{

    template < typename It >
    std::size_t parse_vmir_register(It& ipos, It end)
    {
        if (!skip_symbol_if_is(ipos, end, "%"))
            throw std::logic_error("Expected register symbol");

        return parse_integer(ipos, end);
    }

    template < typename It >
    std::optional< std::size_t > try_parse_vmir_register(It& ipos, It end)
    {
        if (!skip_symbol_if_is(ipos, end, "%"))
            return std::nullopt;

        return parse_integer(ipos, end);
    }

    template < typename It >
    vm_invocation_args parse_invocation_args(It& ipos, It end)
    {
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
            result.named[str.value()] = parse_vmir_register(ipos, end);
            skip_whitespace(ipos, end);
            if (skip_symbol_if_is(ipos, end, ','))
                goto named_arg;
            else if (skip_symbol_if_is(ipos, end, ']'))
                return result;
            else
                throw std::logic_error("Expected ',' or ']' after named argument");
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
            throw std::logic_error("Expected ',' or ']' after positional argument");
    }

    template < typename It >
    std::optional< quxlang::vmir2::access_field > try_parse_access_field(It& ipos, It end)
    {
        if (!skip_keyword_if_is(ipos, end, "ACF"))
            return std::nullopt;

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

    template < typename It >
    std::optional< quxlang::vmir2::invoke > try_parse_invoke(It& ipos, It end)
    {
        if (!skip_keyword_if_is(ipos, end, "IVK"))
            return std::nullopt;

        skip_whitespace(ipos, end);

        quxlang::vmir2::invoke result;

        result.what = parse_type_symbol(ipos, end);
        skip_whitespace(ipos, end);
        consume_symbol(ipos, end, ',');
        skip_whitespace(ipos, end);
        result.target = parse_vmir_register(ipos, end);

        return result;
    }

    template < typename It >
    std::optional< quxlang::vmir2::vm_instruction > try_parse_instruction(It& ipos, It end)
    {
        std::optional< quxlang::vmir2::vm_instruction > result;
        result = try_parse_access_field(ipos, end);
        if (result.has_value())
        {
            return result;
        }
        result = try_parse_invoke(ipos, end);
        if (result.has_value())
        {
            return result;
        }
        return std::nullopt;
    }

} // namespace quxlang::parsers::vmir2

#endif // RPNX_QUXLANG_VMIR2_HEADER
